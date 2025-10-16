#include "ptpch.h"
#include "Proton/Scene/SceneSerializer.h"
#include "Proton/Scene/Scene.h"
#include "Proton/Scene/PrefabManager.h"
#include "Proton/Core/AssetManager.h"
#include "Proton/Scripting/EntityScript.h"
#include "Proton/Scripting/ScriptFactory.h"
#include "Proton/Physics/PhysicsWorld.h"
#include "Proton/Utils/Utils.h"


namespace proton {

	using MetadataComponents = ComponentGroup<
		TagComponent, IDComponent, PrefabComponent, TransformComponent
	>;

	using DataComponents = ComponentGroup<
		NetworkComponent, ScriptComponent, CameraComponent, SpriteComponent,
		ResizableSpriteComponent, CircleRendererComponent, TextComponent,
		RigidbodyComponent, BoxColliderComponent, CircleColliderComponent
	>;

	SceneSerializer::SceneSerializer(Scene* scene)
		: m_Scene(scene)
	{
	}

	json SceneSerializer::SerializeScene()
	{
		PT_CORE_ASSERT(m_Scene);
		if (!m_Scene) return json();

		json j;
		m_State = HierarchyTraversalState();

		j["GameModeClass"] = m_Scene->m_GameModeClassName; 
		j["EnableNetworking"] = m_Scene->m_EnableNetworking; 
		j["EnablePhysics"] = m_Scene->m_EnablePhysics; 
		j["ScreenClearColor"] = m_Scene->m_ClearColor;

		const auto& physicsWorld = m_Scene->m_PhysicsWorld;
		j["GravityForce"] = physicsWorld->m_Gravity;
		j["VelocityIterations"] = physicsWorld->m_PhysicsVelocityIterations;
		j["PositionIterations"] = physicsWorld->m_PhysicsPositionIterations;

		if (Entity entity = m_Scene->GetPrimaryCameraEntity())
		{
			j["PrimaryCameraEntity"] = entity.GetUUID();
		}

		for (Entity entity : m_Scene->m_Root)
		{
			json entityJson = SerializeEntity(entity);
			j["Entities"].push_back(entityJson);
		}
		return j;
	}

	bool SceneSerializer::DeserializeScene(const json& j)
	{
		PT_CORE_ASSERT(m_Scene);
		if (!m_Scene) return false;

		m_State = HierarchyTraversalState();

		m_Scene->SetGameModeByClassName(j["GameModeClass"]);
		m_Scene->m_EnablePhysics = j["EnablePhysics"];
		m_Scene->m_EnableNetworking = j["EnableNetworking"];
		m_Scene->m_ClearColor = j["ScreenClearColor"];

		auto& physicsWorld = m_Scene->m_PhysicsWorld;
		physicsWorld->m_Gravity = j["GravityForce"];
		physicsWorld->m_PhysicsVelocityIterations = j["VelocityIterations"];
		physicsWorld->m_PhysicsPositionIterations = j["PositionIterations"];

		const json& entities = j["Entities"];
		for (auto it = entities.begin(); it != entities.end(); it++)
		{
			DeserializeEntity(*it);
		}

		if (j.contains("PrimaryCameraEntity"))
		{
			UUID uuid = j["PrimaryCameraEntity"];
			m_Scene->SetPrimaryCameraEntity(m_Scene->FindByID(uuid));
		}

		m_Scene->CalculateWorldPositions();
		return true;
	}

	void SceneSerializer::UpdateHierarchyTraversalState()
	{
		auto& s = m_State;
		if (s.IsCurrentPrefab)
		{
			if (s.ParentPrefabLevel > 0 && s.HierarchyLevel > s.ParentPrefabLevel)
				s.IsNestedPrefab = true;
			s.ParentPrefabLevel = s.HierarchyLevel;
		}
		s.HierarchyLevel++;
	}

	bool SceneSerializer::AreDataComponentsSerialized() const
	{
		auto& s = m_State;
		switch (Format)
		{
		case FormatType::Scene:  return !s.IsCurrentPrefab && s.ParentPrefabLevel == -1;
		case FormatType::Prefab: return s.HierarchyLevel == 0 || (!s.IsCurrentPrefab && !s.IsNestedPrefab);
		}
		return false;
	}

	bool SceneSerializer::IsPositionSerialized() const
	{
		auto& s = m_State;
		switch (Format)
		{
		case FormatType::Scene:  return s.HierarchyLevel == 0 || s.ParentPrefabLevel == -1;
		case FormatType::Prefab: return s.HierarchyLevel > 0 && !s.IsNestedPrefab;
		}
		return false;
	}

	json SceneSerializer::SerializeEntity(Entity entity)
	{
		json j;

		m_State.IsCurrentPrefab = entity.HasComponent<PrefabComponent>();

		SerializeComponents(MetadataComponents{}, entity, j, true);

		if (AreDataComponentsSerialized())
		{
			SerializeComponents(DataComponents{}, entity, j);
		}

		SerializeChildEntities(entity, j);

		return j;
	}

	void SceneSerializer::SerializeChildEntities(Entity entity, json& j)
	{
		const auto savedState = m_State;
		UpdateHierarchyTraversalState();

		auto& hierarchy = entity.GetComponent<RelationshipComponent>();
		Entity current(hierarchy.First, m_Scene);
		while (current)
		{
			auto& h = current.GetComponent<RelationshipComponent>();
			j["Entities"].push_back(SerializeEntity(current));
			current = Entity(h.Next, m_Scene);
		}

		m_State = savedState;
	}

	Entity SceneSerializer::DeserializeEntity(const json& j, Entity entity)
	{
		if (!entity)
		{
			UUID uuid = Format == FormatType::Scene ? j["UUID"] : UUID();
			entity = m_Scene->CreateEntityWithUUID(uuid);
		}

		m_State.IsCurrentPrefab = j.contains("PrefabUUID");

		DeserializeComponents(MetadataComponents{}, entity, j, true);

		if (AreDataComponentsSerialized())
		{
			DeserializeComponents(DataComponents{}, entity, j);
			DeserializeChildEntities(entity, j);

			return entity;
		}

		DeserializeChildEntities(entity, j);
		if (m_State.IsCurrentPrefab)
		{
			PrefabManager::DeserializePrefab(entity, (UUID)j["PrefabUUID"]);
		}

		return entity;
	}

	void SceneSerializer::DeserializeChildEntities(Entity entity, const json& j)
	{
		if (!j.contains("Entities"))
			return;

		const json& entities = j["Entities"];

		const auto savedState = m_State;
		UpdateHierarchyTraversalState();

		if (Format == FormatType::Scene)
		{
			for (auto it = entities.rbegin(); it != entities.rend(); it++)
			{
				Entity child = DeserializeEntity(*it);
				entity.AddChildEntity(child, false);
			}
		}
		else if (Format == FormatType::Prefab)
		{
			std::unordered_map<UUID, Entity> sceneEntityMap;
			
			const auto& hierarchy = entity.GetComponent<RelationshipComponent>();
			Entity current(hierarchy.First, m_Scene);
			while (current)
			{
				auto& id = current.GetComponent<IDComponent>();
				sceneEntityMap[id.RefID] = current;

				auto& h = current.GetComponent<RelationshipComponent>();
				current = Entity(h.Next, m_Scene);
			}
			
			for (auto it = entities.rbegin(); it != entities.rend(); it++)
			{
				const json& data = *it;
				UUID prefabRefUUID = data["UUID"];

				if (sceneEntityMap.find(prefabRefUUID) == sceneEntityMap.end())
				{
					Entity child = DeserializeEntity(data);
					auto& id = child.GetComponent<IDComponent>();
					id.RefID = prefabRefUUID;
					entity.AddChildEntity(child, false);
				}
				else
				{
					Entity target = sceneEntityMap[prefabRefUUID];
					DeserializeEntity(data, target);
					sceneEntityMap.erase(prefabRefUUID);
				}
			}

			for (const auto& [refUUID, entity] : sceneEntityMap)
				m_Scene->DestroyEntity(entity);
		}

		m_State = savedState;
	}

	template<>
	void SceneSerializer::Serialize<IDComponent>(Entity entity, const IDComponent& c, json& j)
	{
		if (Format == FormatType::Scene)
		{
			j["UUID"] = c.ID;
			if (m_State.ParentPrefabLevel != -1)
				j["RefUUID"] = c.RefID;
		}
		else if (Format == FormatType::Prefab)
		{
			if (m_State.HierarchyLevel == 0)
				j["UUID"] = entity.GetPrefabUUID();
			else
				j["UUID"] = c.RefID ? c.RefID : c.ID;
		}
	}

	template<>
	void SceneSerializer::Deserialize<IDComponent>(Entity entity, IDComponent& c, const json& j)
	{
		if (Format == FormatType::Scene && m_State.ParentPrefabLevel != -1)
			c.RefID = j["RefUUID"];
	}

	template<>
	void SceneSerializer::Serialize<TagComponent>(Entity entity, const TagComponent& c, json& j)
	{
		j["Tag"] = c.Tag;
	}

	template<>
	void SceneSerializer::Deserialize<TagComponent>(Entity entity, TagComponent& c, const json& j)
	{
		c.Tag = j["Tag"];
	}

	template<>
	void SceneSerializer::Serialize<PrefabComponent>(Entity entity, const PrefabComponent& c, json& j)
	{
		j["PrefabUUID"] = c.PrefabUUID;
	}

	template<>
	void SceneSerializer::Deserialize<PrefabComponent>(Entity entity, PrefabComponent& c, const json& j)
	{
		c.PrefabUUID = j["PrefabUUID"];
	}

	template<>
	void SceneSerializer::Serialize<TransformComponent>(Entity entity, const TransformComponent& c, json& j)
	{
		if (IsPositionSerialized())
		{
			j["Transform"]["Position"] = c.LocalPosition;
		}
		if (AreDataComponentsSerialized())
		{
			j["Transform"]["Rotation"] = c.Rotation;
			j["Transform"]["Scale"] = c.Scale;
		}
	}

	template<>
	void SceneSerializer::Deserialize<TransformComponent>(Entity entity, TransformComponent& c, const json& j)
	{
		if (IsPositionSerialized())
		{
			c.WorldPosition = j["Transform"]["Position"];
			c.LocalPosition = j["Transform"]["Position"];
		}
		if (AreDataComponentsSerialized())
		{
			c.Scale = j["Transform"]["Scale"];
			c.Rotation = j["Transform"]["Rotation"];
		}
	}

	template<>
	void SceneSerializer::Serialize<SpriteComponent>(Entity entity, const SpriteComponent& c, json& j)
	{
		j["Color"] = c.Color;

		if (!c.Sprite)
			return;

		j["Texture"] = Utils::GetRelativeFilepath("content/textures/", c.Sprite.GetTexture()->GetFilepath());
		j["FilterMode"] = c.Sprite.GetTexture()->GetFilterMode();
		j["Flip"] = c.Sprite.m_MirrorFlip;

		if (!c.Sprite.m_TextureAtlas)
			return;

		j["TilePos"] = c.Sprite.m_TileIndex;
		j["TileSize"] = c.Sprite.m_TileCount;
	}

	template<>
	void SceneSerializer::Deserialize<SpriteComponent>(Entity entity, SpriteComponent& c, const json& j)
	{
		c.Color = j["Color"];

		if (!j.contains("Texture"))
			return;

		const auto& texture = AssetManager::GetTexture(j["Texture"]);
		if (!texture)
			return;

		texture->SetFilterMode(j["FilterMode"]);
		c.Sprite.SetTexture(texture);
		c.Sprite.m_MirrorFlip = j["Flip"];

		if (!j.contains("TilePos"))
			return;

		const auto& textureAtlas = AssetManager::GetTextureAtlas(j["Texture"]);
		if (!textureAtlas)
			return;

		c.Sprite.SetTextureAtlas(textureAtlas);
		c.Sprite.SetTileIndex(j["TilePos"]);
		c.Sprite.SetTileCount(j["TileSize"]);
	}

	template<>
	void SceneSerializer::Serialize<ResizableSpriteComponent>(Entity entity, const ResizableSpriteComponent& c, json& j)
	{
		auto& sprite = c.ResizableSprite;
		auto& textureAtlas = sprite.GetTextureAtlas();

		j["TileScale"] = sprite.m_CellScale;
		j["Edges"] = sprite.GetEdgesBitset();
		j["Offset"] = sprite.m_PatternOffset;
		j["PatternSize"] = sprite.m_PatternSize;
		j["Color"] = c.Color;

		if (textureAtlas)
			j["Spritesheet"] = Utils::GetRelativeFilepath("content/textures/", textureAtlas->GetFilepath());
	}

	template<>
	void SceneSerializer::Deserialize<ResizableSpriteComponent>(Entity entity, ResizableSpriteComponent& c, const json& j)
	{
		auto& sprite = c.ResizableSprite;
		sprite.m_EdgesBitset = j["Edges"];
		sprite.m_CellScale = j["TileScale"];
		sprite.m_PatternOffset = j["Offset"];
		sprite.m_PatternSize = j["PatternSize"];
		c.Color = j["Color"];

		if (j.contains("Spritesheet"))
			sprite.m_TextureAtlas = AssetManager::GetTextureAtlas(j["Spritesheet"]);

		const auto& transform = entity.GetComponent<TransformComponent>();
		sprite.Generate(transform.Scale);
	}

	template<>
	void SceneSerializer::Serialize<CircleRendererComponent>(Entity entity, const CircleRendererComponent& c, json& j)
	{
		j["Thickness"] = c.Thickness;
		j["Fade"] = c.Fade;
		j["Color"] = c.Color;
	}

	template<>
	void SceneSerializer::Deserialize<CircleRendererComponent>(Entity entity, CircleRendererComponent& c, const json& j)
	{
		c.Thickness = j["Thickness"];
		c.Fade = j["Fade"];
		c.Color = j["Color"];
	}

	template<>
	void SceneSerializer::Serialize<TextComponent>(Entity entity, const TextComponent& c, json& j)
	{
		j["TextString"] = c.TextString;
		j["Kerning"] = c.Kerning;
		j["LineSpacing"] = c.LineSpacing;
		j["Color"] = c.Color;
		j["Hidden"] = c.Hidden;
	}

	template<>
	void SceneSerializer::Deserialize<TextComponent>(Entity entity, TextComponent& c, const json& j)
	{
		c.TextString = j["TextString"];
		c.Kerning = j["Kerning"];
		c.LineSpacing = j["LineSpacing"];
		c.Hidden = j["Hidden"];
		c.Color = j["Color"];
	}

	template<>
	void SceneSerializer::Serialize<CameraComponent>(Entity entity, const CameraComponent& c, json& j)
	{
		j["PositionOffset"] = c.PositionOffset;
		j["ZoomLevel"] = c.Camera.GetZoomLevel();
	}

	template<>
	void SceneSerializer::Deserialize<CameraComponent>(Entity entity, CameraComponent& c, const json& j)
	{
		c.PositionOffset = j["PositionOffset"];
		c.Camera.SetZoomLevel(j["ZoomLevel"]);
	}

	template<>
	void SceneSerializer::Serialize<RigidbodyComponent>(Entity entity, const RigidbodyComponent& c, json& j)
	{
		j["Type"] = c.Type;
		j["LinearDamping"] = c.LinearDamping;
		j["AngularDamping"] = c.AngularDamping;
		j["GravityScale"] = c.GravityScale;
		j["IsBullet"] = c.IsBullet;
		j["FixedRotation"] = c.FixedRotation;
		j["AttachToParent"] = c.AttachToParent;
	}

	template<>
	void SceneSerializer::Deserialize<RigidbodyComponent>(Entity entity, RigidbodyComponent& c, const json& j)
	{
		c.Type = j["Type"];
		c.LinearDamping = j["LinearDamping"];
		c.AngularDamping = j["AngularDamping"];
		c.GravityScale = j["GravityScale"];
		c.IsBullet = j["IsBullet"];
		c.FixedRotation = j["FixedRotation"];
		c.AttachToParent = j["AttachToParent"];
	}

	template<>
	void SceneSerializer::Serialize<BoxColliderComponent>(Entity entity, const BoxColliderComponent& c, json& j)
	{
		j["Size"] = c.Size;
		j["Offset"] = c.Offset;
		j["Friction"] = c.Material.Friction;
		j["Restitution"] = c.Material.Restitution;
		j["RestitutionThreshold"] = c.Material.RestitutionThreshold;
		j["Density"] = c.Material.Density;
		j["IsSensor"] = c.IsSensor;
		j["AttachToParent"] = c.AttachToParent;
	}

	template<>
	void SceneSerializer::Deserialize<BoxColliderComponent>(Entity entity, BoxColliderComponent& c, const json& j)
	{
		c.Size = j["Size"];
		c.Offset = j["Offset"];
		c.Material.Friction = j["Friction"];
		c.Material.Restitution = j["Restitution"];
		c.Material.RestitutionThreshold = j["RestitutionThreshold"];
		c.Material.Density = j["Density"];
		c.IsSensor = j["IsSensor"];
		c.AttachToParent = j["AttachToParent"];
	}

	template<>
	void SceneSerializer::Serialize<CircleColliderComponent>(Entity entity, const CircleColliderComponent& c, json& j)
	{
		j["Offset"] = c.Offset;
		j["Radius"] = c.Radius;
		j["Friction"] = c.Material.Friction;
		j["Restitution"] = c.Material.Restitution;
		j["RestitutionThreshold"] = c.Material.RestitutionThreshold;
		j["Density"] = c.Material.Density;
		j["IsSensor"] = c.IsSensor;
		j["AttachToParent"] = c.AttachToParent;
	}

	template<>
	void SceneSerializer::Deserialize<CircleColliderComponent>(Entity entity, CircleColliderComponent& c, const json& j)
	{
		c.Offset = j["Offset"];
		c.Radius = j["Radius"];
		c.Material.Friction = j["Friction"];
		c.Material.Restitution = j["Restitution"];
		c.Material.RestitutionThreshold = j["RestitutionThreshold"];
		c.Material.Density = j["Density"];
		c.IsSensor = j["IsSensor"];
		c.AttachToParent = j["AttachToParent"];
	}

	template<>
	void SceneSerializer::Serialize<NetworkComponent>(Entity entity, const NetworkComponent& c, json& j)
	{
		j["SimulateOnClient"] = c.SimulateOnClient;
		j["SyncMethod"] = NetSyncMethodToString(c.NetTransform.Method);
		j["CullDistance"] = c.NetTransform.CullDistance;
		j["TeleportThreshold"] = c.NetTransform.TeleportThreshold;
		j["ReconcileThreshold"] = c.NetTransform.ReconcileThreshold;
		j["ReconcileMaxTime"] = c.NetTransform.ReconcileMaxTime;
	}

	template<>
	void SceneSerializer::Deserialize<NetworkComponent>(Entity entity, NetworkComponent& c, const json& j)
	{
		c.SimulateOnClient = j["SimulateOnClient"];
		c.NetTransform.Method = StringToNetSyncMethod(j["SyncMethod"]);
		c.NetTransform.CullDistance = j["CullDistance"];
		c.NetTransform.TeleportThreshold = j["TeleportThreshold"];
		c.NetTransform.ReconcileThreshold = j["ReconcileThreshold"];
		c.NetTransform.ReconcileMaxTime = j["ReconcileMaxTime"];
	}

	template<>
	void SceneSerializer::Serialize<ScriptComponent>(Entity entity, const ScriptComponent& c, json& j)
	{
		for (auto& [scriptClassName, scriptInstance] : c.Scripts)
		{
			json scriptJson;
			scriptJson["ClassName"] = scriptClassName;

			for (auto& [fieldName, fieldData] : scriptInstance->m_ScriptFields)
			{
				json& fieldJson = scriptJson["Fields"][fieldName];
				void* valuePtr = fieldData.ValuePtr;

				#define WRITE_FIELD_DATA(EnumType, Type) \
					case ScriptFieldType::EnumType: fieldJson = *(Type*)valuePtr; break

				switch (fieldData.Type)
				{
					WRITE_FIELD_DATA(Float, float);
					WRITE_FIELD_DATA(Float2, glm::vec2);
					WRITE_FIELD_DATA(Float3, glm::vec3);
					WRITE_FIELD_DATA(Float4, glm::vec4);
					WRITE_FIELD_DATA(Int, int);
					WRITE_FIELD_DATA(Int2, glm::ivec2);
					WRITE_FIELD_DATA(Int3, glm::ivec3);
					WRITE_FIELD_DATA(Int4, glm::ivec4);
					WRITE_FIELD_DATA(Bool, bool);
					WRITE_FIELD_DATA(String, std::string);
				}
			}

			j.push_back(scriptJson);
		}
	}

	template<>
	void SceneSerializer::Deserialize<ScriptComponent>(Entity entity, ScriptComponent& c, const json& j)
	{
		for (auto& scriptJson : j)
		{
			std::string className = scriptJson["ClassName"];
			EntityScript* script = ScriptFactory::Get().AddScriptToEntity(entity, className);

			if (script == nullptr || !scriptJson.contains("Fields"))
				continue;

			for (auto& [fieldName, fieldValue] : scriptJson["Fields"].items())
			{
				if (script->m_ScriptFields.find(fieldName) == script->m_ScriptFields.end())
					continue;

				const ScriptField& scriptField = script->m_ScriptFields[fieldName];
				void* valuePtr = scriptField.ValuePtr;

				#define READ_FIELD_DATA(EnumType, Type) \
					case ScriptFieldType::EnumType: *(Type*)valuePtr = fieldValue; break

				switch (scriptField.Type)
				{
				READ_FIELD_DATA(Float, float);
				READ_FIELD_DATA(Float2, glm::vec2);
				READ_FIELD_DATA(Float3, glm::vec3);
				READ_FIELD_DATA(Float4, glm::vec4);
				READ_FIELD_DATA(Int, int);
				READ_FIELD_DATA(Int2, glm::ivec2);
				READ_FIELD_DATA(Int3, glm::ivec3);
				READ_FIELD_DATA(Int4, glm::ivec4);
				READ_FIELD_DATA(Bool, bool);
				READ_FIELD_DATA(String, std::string);
				}
			}
		}
	}

	std::string SceneSerializer::SerializeSceneToString()
	{
		json j = SerializeScene();
		return j.dump(4);
	}

	bool SceneSerializer::SerializeSceneToFile(std::string_view filepath)
	{
		std::string jsonData = SerializeSceneToString();
		if (jsonData.size() == 0)
			return false;

		std::ofstream out(filepath.data());
		out << jsonData;
		out.close();
		return true;
	}

	bool SceneSerializer::DeserializeSceneFromString(std::string_view jsonData)
	{
		if (jsonData.size() == 0)
			return false;

		json j = json::parse(jsonData);
		return DeserializeScene(j);
	}

	bool SceneSerializer::DeserializeSceneFromFile(std::string_view filepath)
	{
		std::string jsonData = Utils::ReadFile(filepath);
		if (jsonData.size() == 0)
			return false;

		return DeserializeSceneFromString(jsonData);
	}

	std::string SceneSerializer::SerializeEntityToString(Entity entity)
	{
		return SerializeEntity(entity).dump(4);
	}

	bool SceneSerializer::SerializeEntityToFile(Entity entity, std::string_view filepath)
	{
		std::string jsonData = SerializeEntityToString(entity);
		if (jsonData.size() == 0)
			return false;

		std::ofstream out(filepath.data());
		out << jsonData;
		out.close();
		return true;
	}

	Entity SceneSerializer::DeserializeEntityFromString(std::string_view jsonData, Entity entity)
	{
		if (jsonData.size() == 0)
			return Entity();

		json j = json::parse(jsonData);
		return DeserializeEntity(j, entity);
	}

	Entity SceneSerializer::DeserializeEntityFromFile(std::string_view filepath, Entity entity)
	{
		std::string jsonData = Utils::ReadFile(filepath);
		if (jsonData.size() == 0)
			return Entity();

		return DeserializeEntityFromString(jsonData);
	}

	template<typename TComponent>
	void SceneSerializer::Serialize(Entity entity, const TComponent& c, json& j)
	{
		static_assert(sizeof(TComponent) == 0);
	}

	template<typename TComponent>
	void SceneSerializer::Deserialize(Entity entity, TComponent& c, const json& j)
	{
		static_assert(sizeof(TComponent) == 0);
	}

	template<typename TComponent>
	static constexpr std::string_view SceneSerializer::GetComponentName()
	{
		std::string_view className = TComponent::_ClassName();
		size_t pos = className.find("Component");
		if (pos) return className.substr(0, pos);
		return className;
	}

	template<typename TComponent>
	inline void SceneSerializer::TrySerialize(Entity entity, json& j, bool useRootObject)
	{
		constexpr std::string_view key = GetComponentName<TComponent>();

		if (entity.HasComponent<TComponent>())
		{
			const auto& component = entity.GetComponent<TComponent>();

			if (!useRootObject)
			{
				Serialize<TComponent>(entity, component, j[key]);
				if (j[key].is_null())
					j.erase(key);
			}
			else
				Serialize<TComponent>(entity, component, j);
		}
	}

	template<typename TComponent>
	inline void SceneSerializer::TryDeserialize(Entity entity, const json& j, bool useRootObject)
	{
		constexpr std::string_view key = GetComponentName<TComponent>();

		if (j.contains(key) || useRootObject)
		{
			if constexpr (std::is_base_of<PrefabComponent, TComponent>())
			{
				if (!j.contains("PrefabUUID"))
					return;
			}
			
			TComponent* component = entity.HasComponent<TComponent>() ?
				&entity.GetComponent<TComponent>() : &entity.AddComponent<TComponent>();

			if (!useRootObject)
				Deserialize<TComponent>(entity, *component, j[key]);
			else
				Deserialize<TComponent>(entity, *component, j);
		}
	}

	template<typename... TComponent>
	void SceneSerializer::SerializeComponents(Entity entity, json& j, bool useRootObject)
	{
		([&]() 
		{
			TrySerialize<TComponent>(entity, j, useRootObject);
		}(), ...);
	}

	template<typename... TComponent>
	void SceneSerializer::DeserializeComponents(Entity entity, const json& j, bool useRootObject)
	{
		([&]() 
		{
			TryDeserialize<TComponent>(entity, j, useRootObject);
		}(), ...);
	}

	template<typename ...TComponent>
	void SceneSerializer::SerializeComponents(ComponentGroup<TComponent...>, Entity entity, json& j, bool useRootObject)
	{
		SerializeComponents<TComponent...>(entity, j, useRootObject);
	}

	template<typename ...TComponent>
	void SceneSerializer::DeserializeComponents(ComponentGroup<TComponent...>, Entity entity, const json& j, bool useRootObject)
	{
		DeserializeComponents<TComponent...>(entity, j, useRootObject);
	}

}
