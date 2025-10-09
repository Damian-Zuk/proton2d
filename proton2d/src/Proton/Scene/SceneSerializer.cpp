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
		ScriptComponent, NetworkComponent, CameraComponent, SpriteComponent,
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

		const auto& physicsWorld = m_Scene->m_PhysicsWorld;
		j["GravityForce"] = physicsWorld->m_Gravity;
		j["VelocityIterations"] = physicsWorld->m_PhysicsVelocityIterations;
		j["PositionIterations"] = physicsWorld->m_PhysicsPositionIterations;

		const glm::vec4& col = m_Scene->m_ClearColor;
		j["ScreenClearColor"] = { col.r, col.g, col.b, col.a };

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

		auto& physicsWorld = m_Scene->m_PhysicsWorld;
		physicsWorld->m_Gravity = j["GravityForce"];
		physicsWorld->m_PhysicsVelocityIterations = j["VelocityIterations"];
		physicsWorld->m_PhysicsPositionIterations = j["PositionIterations"];
		
		const json& col = j["ScreenClearColor"];
		m_Scene->m_ClearColor = { col[0], col[1], col[2], col[3] };

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

	inline bool SceneSerializer::AreDataComponentsSerialized() const
	{
		auto& s = m_State;
		switch (Format)
		{
		case FormatType::Scene:  return !s.IsCurrentPrefab && s.ParentPrefabLevel == -1;
		case FormatType::Prefab: return s.HierarchyLevel == 0 || (!s.IsCurrentPrefab && !s.IsNestedPrefab);
		}
		return false;
	}

	inline bool proton::SceneSerializer::IsPositionSerialized() const
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

		TrySerialize(MetadataComponents{}, entity, j, true);

		if (AreDataComponentsSerialized())
		{
			TrySerialize(DataComponents{}, entity, j);
		}

		SerializeChildEntities(entity, j);

		return j;
	}

	void SceneSerializer::SerializeChildEntities(Entity entity, json& j)
	{
		const auto savedState = m_State;
		UpdateHierarchyTraversalState();

		auto& hierarchy = entity.GetComponent<RelationshipComponent>();
		if (hierarchy.ChildrenCount > 0)
		{
			Entity current(hierarchy.First, entity.GetScene());
			while (current)
			{
				auto& h = current.GetComponent<RelationshipComponent>();
				j["Entities"].push_back(SerializeEntity(current));
				current = Entity(h.Next, entity.GetScene());
			}
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

		TryDeserialize(MetadataComponents{}, entity, j, true);

		if (AreDataComponentsSerialized())
		{
			TryDeserialize(DataComponents{}, entity, j);
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
			Entity current(hierarchy.First, entity.GetScene());
			while (current)
			{
				auto& id = current.GetComponent<IDComponent>();
				sceneEntityMap[id.PrefabRefID] = current;

				auto& h = current.GetComponent<RelationshipComponent>();
				current = Entity(h.Next, entity.GetScene());
			}
			
			for (auto it = entities.rbegin(); it != entities.rend(); it++)
			{
				const json& data = *it;
				UUID prefabRefUUID = data["UUID"];

				if (sceneEntityMap.find(prefabRefUUID) != sceneEntityMap.end())
				{
					Entity target = sceneEntityMap[prefabRefUUID];
					DeserializeEntity(data, target);
				}
				else
				{
					Entity child = DeserializeEntity(data);
					auto& id = child.GetComponent<IDComponent>();
					id.PrefabRefID = prefabRefUUID;
					entity.AddChildEntity(child, false);
				}
			}
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
				j["PrefabRefUUID"] = c.PrefabRefID;
		}
		else if (Format == FormatType::Prefab)
		{
			if (m_State.HierarchyLevel == 0)
				j["UUID"] = entity.GetPrefabUUID();
			else
				j["UUID"] = c.PrefabRefID;
		}
	}

	template<>
	void SceneSerializer::Deserialize<IDComponent>(Entity entity, IDComponent& c, const json& j)
	{
		if (Format == FormatType::Scene && m_State.ParentPrefabLevel != -1)
			c.PrefabRefID = j["PrefabRefUUID"];
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
			const glm::vec3& pos = c.LocalPosition;
			j["Transform"]["Position"] = {pos.x, pos.y, pos.z};
		}
		if (AreDataComponentsSerialized())
		{
			j["Transform"]["Rotation"] = c.Rotation;
			j["Transform"]["Scale"] = { c.Scale.x, c.Scale.y };
		}
	}

	template<>
	void SceneSerializer::Deserialize<TransformComponent>(Entity entity, TransformComponent& c, const json& j)
	{
		if (IsPositionSerialized())
		{
			const json& pos = j["Transform"]["Position"];
			c.WorldPosition = { pos[0], pos[1], pos[2] };
			c.LocalPosition = { pos[0], pos[1], pos[2] };
		}
		if (AreDataComponentsSerialized())
		{
			const json& scale = j["Transform"]["Scale"];
			const json& rotation = j["Transform"]["Rotation"];
			c.Scale = { scale[0], scale[1] };
			c.Rotation = rotation;
		}
	}

	template<>
	void SceneSerializer::Serialize<SpriteComponent>(Entity entity, const SpriteComponent& c, json& j)
	{
		const auto& sprite = c.Sprite;
		auto& color = c.Color;
		if (sprite)
		{
			j["Texture"] = Utils::GetRelativeFilepath("content/textures/", sprite.GetTexture()->GetPath());
			j["FilterMode"] = sprite.GetTexture()->GetFilterMode();
			j["Flip"] = { sprite.m_MirrorFlip.x, sprite.m_MirrorFlip.y };

			if (sprite.m_Spritesheet)
			{
				j["TilePos"] = { sprite.m_TilePos.x, sprite.m_TilePos.y };
				j["TileSize"] = { sprite.m_TileSize.x, sprite.m_TileSize.y };
			}
		}
		j["Color"] = { color.r, color.g, color.b, color.a };
	}

	template<>
	void SceneSerializer::Deserialize<SpriteComponent>(Entity entity, SpriteComponent& c, const json& j)
	{
		const json& col = j["Color"];
		c.Color = { col[0], col[1], col[2], col[3] };

		if (!j.contains("Texture"))
			return;

		std::string texturePath = j["Texture"];
		const auto& texture = AssetManager::GetTexture(texturePath);
		if (!texture)
			return;

		texture->m_FilterMode = j["FilterMode"];
		c.Sprite.SetTexture(texture);
		c.Sprite.m_MirrorFlip.x = j["Flip"][0];
		c.Sprite.m_MirrorFlip.y = j["Flip"][1];

		if (!j.contains("TilePos"))
			return;

		const auto& spritesheet = AssetManager::GetSpritesheet(texturePath);
		if (!spritesheet)
			return;

		c.Sprite.SetSpritesheet(spritesheet);
		c.Sprite.SetTile(j["TilePos"][0], j["TilePos"][1]);
		c.Sprite.SetTileSize(j["TileSize"][0], j["TileSize"][1]);
	}

	template<>
	void SceneSerializer::Serialize<ResizableSpriteComponent>(Entity entity, const ResizableSpriteComponent& c, json& j)
	{
		auto& sprite = c.ResizableSprite;
		auto& spritesheet = sprite.GetSpritesheet();
		const glm::vec4& col = c.Color;

		j["Width"] = sprite.m_CellCount.x;
		j["Height"] = sprite.m_CellCount.y;
		j["TileScale"] = sprite.m_CellScale;
		j["Edges"] = sprite.GetEdgesBitset();
		j["Offset"] = { sprite.m_PatternOffset.x, sprite.m_PatternOffset.y };
		j["PatternSize"] = { sprite.m_PatternSize.x, sprite.m_PatternSize.y };
		j["Color"] = { col.r, col.g, col.b, col.a };

		if (spritesheet)
			j["Spritesheet"] = Utils::GetRelativeFilepath("content/textures/", spritesheet->GetTexture()->GetPath());
	}

	template<>
	void SceneSerializer::Deserialize<ResizableSpriteComponent>(Entity entity, ResizableSpriteComponent& c, const json& j)
	{
		auto& sprite = c.ResizableSprite;
		sprite.m_EdgesBitset = j["Edges"];
		sprite.m_CellScale = j["TileScale"];
		const json& offset = j["Offset"];
		sprite.m_PatternOffset = { offset[0], offset[1] };
		const json& patternSize = j["PatternSize"];
		sprite.m_PatternSize = { patternSize[0], patternSize[1] };

		if (j.contains("Spritesheet"))
			sprite.m_Spritesheet = AssetManager::GetSpritesheet(j["Spritesheet"]);

		auto& transform = entity.GetComponent<TransformComponent>();
		sprite.Generate(transform.Scale);

		const json& col = j["Color"];
		c.Color = { col[0], col[1], col[2], col[3] };
	}

	template<>
	void SceneSerializer::Serialize<CircleRendererComponent>(Entity entity, const CircleRendererComponent& c, json& j)
	{
		const glm::vec4& col = c.Color;
		j["Thickness"] = c.Thickness;
		j["Fade"] = c.Fade;
		j["Color"] = { col.r, col.g, col.b, col.a };
	}

	template<>
	void SceneSerializer::Deserialize<CircleRendererComponent>(Entity entity, CircleRendererComponent& c, const json& j)
	{
		const json& col = j["Color"];
		c.Thickness = j["Thickness"];
		c.Fade = j["Fade"];
		c.Color = { col[0], col[1], col[2], col[3] };
	}

	template<>
	void SceneSerializer::Serialize<TextComponent>(Entity entity, const TextComponent& c, json& j)
	{
		const glm::vec4& col = c.Color;
		j["TextString"] = c.TextString;
		j["Kerning"] = c.Kerning;
		j["LineSpacing"] = c.LineSpacing;
		j["Color"] = { col.r, col.g, col.b, col.a };
		j["Hidden"] = c.Hidden;
	}

	template<>
	void SceneSerializer::Deserialize<TextComponent>(Entity entity, TextComponent& c, const json& j)
	{
		const json& col = j["Color"];
		c.TextString = j["TextString"];
		c.Kerning = j["Kerning"];
		c.LineSpacing = j["LineSpacing"];
		c.Hidden = j["Hidden"];
		c.Color = { col[0], col[1], col[2], col[3] };
	}

	template<>
	void SceneSerializer::Serialize<CameraComponent>(Entity entity, const CameraComponent& c, json& j)
	{
		j["PositionOffset"] = { c.PositionOffset.x, c.PositionOffset.y };
		j["ZoomLevel"] = c.Camera.GetZoomLevel();
	}

	template<>
	void SceneSerializer::Deserialize<CameraComponent>(Entity entity, CameraComponent& c, const json& j)
	{
		const json& offset = j["PositionOffset"];
		c.PositionOffset = { offset[0], offset[1] };
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
		j["Size"] = { c.Size.x,   c.Size.y };
		j["Offset"] = { c.Offset.x, c.Offset.y };
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
		const json& size = j["Size"];
		const json& offset = j["Offset"];
		c.Size = { size[0], size[1] };
		c.Offset = { offset[0], offset[1] };
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
		j["Offset"] = { c.Offset.x, c.Offset.y };
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
		const json& offset = j["Offset"];
		c.Offset = { offset[0], offset[1] };
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
			json scriptJ;
			scriptJ["ClassName"] = scriptClassName;

			for (auto& [fieldName, fieldData] : scriptInstance->m_ScriptFields)
			{
				json fieldJ;
				fieldJ["FieldName"] = fieldName;

				switch (fieldData.Type)
				{
				case ScriptFieldType::Float:
					fieldJ["Value"] = *(float*)fieldData.ValuePtr;
					break;
				case ScriptFieldType::Float2:
				{
					const glm::vec2& data = *(glm::vec2*)fieldData.ValuePtr;
					fieldJ["Value"] = { data.x, data.y };
					break;
				}
				case ScriptFieldType::Float3:
				{
					const glm::vec3& data = *(glm::vec3*)fieldData.ValuePtr;
					fieldJ["Value"] = { data.x, data.y, data.z };
					break;
				}
				case ScriptFieldType::Float4:
				{
					const glm::vec4& data = *(glm::vec4*)fieldData.ValuePtr;
					fieldJ["Value"] = { data.x, data.y, data.z, data.a };
					break;
				}
				case ScriptFieldType::Int:
					fieldJ["Value"] = *(int*)fieldData.ValuePtr;
					break;
				case ScriptFieldType::Int2:
				{
					const glm::ivec2& data = *(glm::ivec2*)fieldData.ValuePtr;
					fieldJ["Value"] = { data.x, data.y };
					break;
				}
				case ScriptFieldType::Int3:
				{
					const glm::ivec3& data = *(glm::ivec3*)fieldData.ValuePtr;
					fieldJ["Value"] = { data.x, data.y, data.z };
					break;
				}
				case ScriptFieldType::Int4:
				{
					const glm::ivec4& data = *(glm::ivec4*)fieldData.ValuePtr;
					fieldJ["Value"] = { data.x, data.y, data.z, data.a };
					break;
				}
				case ScriptFieldType::Bool:
					fieldJ["Value"] = *(bool*)fieldData.ValuePtr;
					break;
				case ScriptFieldType::String:
					fieldJ["Value"] = *(std::string*)fieldData.ValuePtr;
					break;
				}

				scriptJ["Fields"].push_back(fieldJ);
			}

			j.push_back(scriptJ);
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

			for (auto& field : scriptJson["Fields"])
			{
				std::string fieldName = field["FieldName"];
				if (script->m_ScriptFields.find(fieldName) == script->m_ScriptFields.end())
					continue;

				ScriptField& scriptField = script->m_ScriptFields[fieldName];
				const json& value = field["Value"];

				switch (scriptField.Type)
				{
				case ScriptFieldType::Float:
					*(float*)scriptField.ValuePtr = value;
					break;
				case ScriptFieldType::Float2:
					*(glm::vec2*)scriptField.ValuePtr = { value[0], value[1] };
					break;
				case ScriptFieldType::Float3:
					*(glm::vec3*)scriptField.ValuePtr = { value[0], value[1], value[2] };
					break;
				case ScriptFieldType::Float4:
					*(glm::vec4*)scriptField.ValuePtr = { value[0], value[1], value[2], value[3] };
					break;
				case ScriptFieldType::Int:
					*(int*)scriptField.ValuePtr = value;
					break;
				case ScriptFieldType::Int2:
					*(glm::ivec2*)scriptField.ValuePtr = { value[0], value[1] };
					break;
				case ScriptFieldType::Int3:
					*(glm::ivec3*)scriptField.ValuePtr = { value[0], value[1], value[2] };
					break;
				case ScriptFieldType::Int4:
					*(glm::ivec4*)scriptField.ValuePtr = { value[0], value[1], value[2], value[3] };
					break;
				case ScriptFieldType::Bool:
					*(bool*)scriptField.ValuePtr = value;
					break;
				case ScriptFieldType::String:
					*(std::string*)scriptField.ValuePtr = value;
					break;
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
	inline void SceneSerializer::TrySerialize(std::string_view key, Entity entity, json& j, bool useRootObject)
	{
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
	inline void SceneSerializer::TryDeserialize(std::string_view key, Entity entity, const json& j, bool useRootObject)
	{
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
	void SceneSerializer::TrySerialize(Entity entity, json& j, bool useRootObject)
	{
		([&]() 
		{
			TrySerialize<TComponent>(TComponent::_ClassName(), entity, j, useRootObject);
		}(), ...);
	}

	template<typename... TComponent>
	void SceneSerializer::TryDeserialize(Entity entity, const json& j, bool useRootObject)
	{
		([&]() 
		{
			TryDeserialize<TComponent>(TComponent::_ClassName(), entity, j, useRootObject);
		}(), ...);
	}

	template<typename ...TComponent>
	void SceneSerializer::TrySerialize(ComponentGroup<TComponent...>, Entity entity, json& j, bool useRootObject)
	{
		TrySerialize<TComponent...>(entity, j, useRootObject);
	}

	template<typename ...TComponent>
	void SceneSerializer::TryDeserialize(ComponentGroup<TComponent...>, Entity entity, const json& j, bool useRootObject)
	{
		TryDeserialize<TComponent...>(entity, j, useRootObject);
	}

}
