#include "ptpch.h"
#include "Proton/Scene/SceneSerializer.h"
#include "Proton/Scene/Scene.h"
#include "Proton/Scene/PrefabManager.h"
#include "Proton/Core/AssetManager.h"
#include "Proton/Scripting/EntityScript.h"
#include "Proton/Scripting/ScriptFactory.h"
#include "Proton/Physics/PhysicsWorld.h"
#include "Proton/Utils/Utils.h"

#include <fstream>

namespace proton {

	static std::string GetFilepathRelative(const std::string& dir, const std::string& filepath)
	{
		return filepath.substr(dir.size(), filepath.size() - dir.size());;
	}

	static inline double rf(float f)
	{
		return std::round((double)f * 100000) / 100000;
	}

	SceneSerializer::SceneSerializer(Scene* scene, FormatType format)
		: Format(format), m_Scene(scene)
	{
	}

	std::string SceneSerializer::SerializeScene()
	{
		PT_CORE_VERIFY(m_Scene, "Invalid Scene");
		if (!m_Scene)
			return std::string();

		const glm::vec4& col = m_Scene->m_ClearColor;

		json data = {
			{ "GameModeClass",      m_Scene->m_GameModeClassName },
			{ "EnableNetworking",   m_Scene->m_EnableNetworking },
			{ "EnablePhysics",      m_Scene->m_EnablePhysics },
			{ "GravityForce",       rf(m_Scene->m_PhysicsWorld->m_Gravity) },
			{ "VelocityIterations", m_Scene->m_PhysicsWorld->m_PhysicsVelocityIterations },
			{ "PositionIterations", m_Scene->m_PhysicsWorld->m_PhysicsPositionIterations },
			{ "ScreenClearColor", { rf(col.r), rf(col.g), rf(col.b), rf(col.a) } }
		};

		Entity primaryCameraEntity = m_Scene->GetPrimaryCameraEntity();
		if (primaryCameraEntity.IsValid())
		{
			uint64_t id = m_Scene->GetPrimaryCameraEntity().GetUUID();
			data["PrimaryCameraEntity"] = id;
		}

		for (Entity entity : m_Scene->m_Root)
		{
			data["Entities"].push_back(SerializeEntity(entity));
		}

		ResetState();
		return data.dump(4);
	}

	bool SceneSerializer::DeserializeScene(const std::string& jsonData)
	{
		PT_CORE_VERIFY(m_Scene, "Invalid Scene");
		if (!m_Scene)
			return false;

		json j = json::parse(jsonData);

		m_Scene->SetGameModeByClassName(j["GameModeClass"]);
		m_Scene->m_EnablePhysics = j["EnablePhysics"];
		m_Scene->m_EnableNetworking = j["EnableNetworking"];
		m_Scene->m_PhysicsWorld->m_Gravity = j["GravityForce"];
		m_Scene->m_PhysicsWorld->m_PhysicsVelocityIterations = j["VelocityIterations"];
		m_Scene->m_PhysicsWorld->m_PhysicsPositionIterations = j["PositionIterations"];
		
		const json& col = j["ScreenClearColor"];
		m_Scene->m_ClearColor = { col[0], col[1], col[2], col[3] };

		const json& entities = j["Entities"];
		for (auto it = entities.begin(); it != entities.end(); it++)
		{
			DeserializeEntity(*it);
		}

		if (j.contains("PrimaryCameraEntity"))
		{
			UUID id{ j["PrimaryCameraEntity"] };
			m_Scene->SetPrimaryCameraEntity(m_Scene->FindByID(id));
		}

		m_Scene->CalculateWorldPositions();

		ResetState();
		return true;
	}

	inline bool SceneSerializer::AreComponentsSerialized(bool isPrefabEntity) const
	{
		return (Format == FormatType::Scene && (!isPrefabEntity && m_ParentPrefabLevel == -1)) ||
			(Format == FormatType::Prefab && (!isPrefabEntity || m_ParentPrefabLevel == -1));
	}

	inline bool proton::SceneSerializer::IsPositionSerialized() const
	{
		return (Format == FormatType::Scene && (m_HierarchyLevel == 0 || m_ParentPrefabLevel == -1))
			|| (Format == FormatType::Prefab && m_HierarchyLevel > 0 && m_NestedPrefabsCount == 0);
	}

	inline bool SceneSerializer::IsRotationAndScaleSerialized(bool isPrefabEntity) const
	{
		return (Format == FormatType::Scene && (!isPrefabEntity && m_ParentPrefabLevel == -1))
			|| (Format == FormatType::Prefab && (m_HierarchyLevel == 0 || (!isPrefabEntity && m_NestedPrefabsCount == 0)));
	}

	json SceneSerializer::SerializeEntity(Entity entity)
	{
		json j;

		const auto& id = entity.GetComponent<IDComponent>();
		const bool isPrefabEntity = entity.HasComponent<PrefabComponent>();
		UUID prefabUUID = isPrefabEntity ? entity.GetComponent<PrefabComponent>().PrefabUUID : 0;

		j["Tag"] = entity.GetTag();

		if (Format == FormatType::Scene)
		{
			j["UUID"] = (uint64_t)id.ID;
			if (m_ParentPrefabLevel != -1)
			{
				j["PrefabChildRefUUID"] = (uint64_t)id.PrefabChildRefID;
			}
		}
		else if (Format == FormatType::Prefab)
		{
			j["UUID"] = (uint64_t)(m_HierarchyLevel > 0 ? id.PrefabChildRefID : prefabUUID);
		}

		if (isPrefabEntity)
		{
			j["PrefabUUID"] = (uint64_t)prefabUUID;
		}

		TrySerialize<TransformComponent>("Transform", entity, j);

		if (!AreComponentsSerialized(isPrefabEntity))
		{
			SerializeChildEntities(entity, j);
			return j;
		}

#define _TrySerialize(T) TrySerialize<T>(#T, entity, j)
		_TrySerialize(ScriptComponent);
		_TrySerialize(NetworkComponent);
		_TrySerialize(CameraComponent);
		_TrySerialize(SpriteComponent);
		_TrySerialize(ResizableSpriteComponent);
		_TrySerialize(CircleRendererComponent);
		_TrySerialize(TextComponent);
		_TrySerialize(RigidbodyComponent);
		_TrySerialize(BoxColliderComponent);
		_TrySerialize(CircleColliderComponent);

		SerializeChildEntities(entity, j);
		return j;
	}

	void SceneSerializer::SerializeChildEntities(Entity entity, json& out)
	{
		int32_t initialParentLevel = m_ParentPrefabLevel;
		int32_t initialNestedPrefabs = m_NestedPrefabsCount;
		int32_t initialHierarchyLevel = m_HierarchyLevel;

		if (entity.HasComponent<PrefabComponent>())
		{
			if (m_ParentPrefabLevel > 0 && m_HierarchyLevel > m_ParentPrefabLevel)
				m_NestedPrefabsCount++;
			m_ParentPrefabLevel = m_HierarchyLevel;
		}
		m_HierarchyLevel++;

		auto& relationship = entity.GetComponent<RelationshipComponent>();
		if (relationship.ChildrenCount)
		{
			entt::entity current = relationship.First;
			while (current != entt::null)
			{
				Entity child{ current, entity.m_Scene };
				auto& rc = child.GetComponent<RelationshipComponent>();
				out["Entities"].push_back(SerializeEntity(child));
				current = rc.Next;
			}
		}

		// Restore initial values
		m_ParentPrefabLevel = initialParentLevel;
		m_NestedPrefabsCount = initialNestedPrefabs;
		m_HierarchyLevel = initialHierarchyLevel;
	}

	Entity SceneSerializer::DeserializeEntity(const json& j, Entity entity)
	{
		const bool isPrefabEntity = j.contains("PrefabUUID");
		const UUID prefabUUID = isPrefabEntity ? (uint64_t)j["PrefabUUID"] : 0;

		if (!entity)
		{
			UUID uuid = Format == FormatType::Scene ? (uint64_t)j["UUID"] : UUID();
			entity = m_Scene->CreateEntityWithUUID(uuid);
		}

		if (Format == FormatType::Scene && m_ParentPrefabLevel != -1)
		{
			auto& id = entity.GetComponent<IDComponent>();
			id.PrefabChildRefID = (uint64_t)j["PrefabChildRefUUID"];
		}

		auto& tag = entity.GetComponent<TagComponent>();
		tag.Tag = j["Tag"];

		if (isPrefabEntity)
		{
			auto& pc = entity.AddOrReplaceComponent<PrefabComponent>();
			pc.PrefabUUID = prefabUUID;
		}

		TryDeserialize<TransformComponent>("Transform", entity, j);

		if (!AreComponentsSerialized(isPrefabEntity))
		{
			DeserializeChildEntities(entity, j);
			if (isPrefabEntity)
			{
				PrefabManager::DeserializePrefab(entity, prefabUUID);
			}
			return entity;
		}

#define _TryDeserialize(T) TryDeserialize<T>(#T, entity, j)
		_TryDeserialize(ScriptComponent);
		_TryDeserialize(NetworkComponent);
		_TryDeserialize(CameraComponent);
		_TryDeserialize(SpriteComponent);
		_TryDeserialize(ResizableSpriteComponent);
		_TryDeserialize(CircleRendererComponent);
		_TryDeserialize(TextComponent);
		_TryDeserialize(RigidbodyComponent);
		_TryDeserialize(BoxColliderComponent);
		_TryDeserialize(CircleColliderComponent);

		DeserializeChildEntities(entity, j);
		return entity;
	}

	void SceneSerializer::DeserializeChildEntities(Entity entity, const json& data)
	{
		if (!data.contains("Entities"))
			return;

		int32_t initialParentLevel = m_ParentPrefabLevel;
		int32_t initialNestedPrefabs = m_NestedPrefabsCount;
		int32_t initialHierarchyLevel = m_HierarchyLevel;

		if (entity.HasComponent<PrefabComponent>())
		{
			if (m_ParentPrefabLevel > 0 && m_HierarchyLevel > m_ParentPrefabLevel)
				m_NestedPrefabsCount++;
			m_ParentPrefabLevel = m_HierarchyLevel;
		}
		m_HierarchyLevel++;

		auto& hierarchy = entity.GetComponent<RelationshipComponent>();
		const json& entities = data["Entities"];

		// Create new child entities
		if (hierarchy.ChildrenCount == 0)
		{
			for (auto it = entities.rbegin(); it != entities.rend(); it++)
			{
				const json& data = *it;
				Entity child = DeserializeEntity(data);
				entity.AddChildEntity(child, false);

				if (Format == FormatType::Prefab)
				{
					PT_CORE_ASSERT(data.contains("UUID"));
					auto& id = child.GetComponent<IDComponent>();
					id.PrefabChildRefID = (uint64_t)data["UUID"];
				}
			}
		}
		// Create mapping for existing hierarchy
		else if (Format == FormatType::Prefab)
		{
			std::unordered_map<UUID, Entity> prefabEntityMap;
			Entity current(hierarchy.First, entity.GetScene());
			while (current)
			{
				auto& id = current.GetComponent<IDComponent>();
				prefabEntityMap[id.PrefabChildRefID] = current;
				auto& h = current.GetComponent<RelationshipComponent>();
				current = Entity(h.Next, entity.GetScene());
			}

			// Read Prefab JSON and deserialize to existing entities
			for (auto it = entities.rbegin(); it != entities.rend(); it++)
			{
				const json& entityData = *it;
				std::string keyName = (Format == FormatType::Scene ? "PrefabChildRefUUID" : "UUID");

				PT_CORE_ASSERT(entityData.contains(keyName));
				UUID PrefabChildRefUUID = (uint64_t)entityData[keyName];

				if (prefabEntityMap.find(PrefabChildRefUUID) == prefabEntityMap.end())
				{
					PT_CORE_ERROR("Could not map prefab child uuid {} to entity", PrefabChildRefUUID);
					continue;
				}

				Entity target = prefabEntityMap[PrefabChildRefUUID];
				DeserializeEntity(*it, target);
			}
		}

		m_ParentPrefabLevel = initialParentLevel;
		m_NestedPrefabsCount = initialNestedPrefabs;
		m_HierarchyLevel = initialHierarchyLevel;
	}

	void SceneSerializer::ResetState()
	{
		m_HierarchyLevel = 0;
		m_ParentPrefabLevel = -1;
		m_NestedPrefabsCount = 0;
	}

	template<>
	void SceneSerializer::Serialize<TransformComponent>(Entity entity, const TransformComponent& c, json& j)
	{
		const bool isPrefabEntity = entity.HasComponent<PrefabComponent>();

		if (IsPositionSerialized())
		{
			const glm::vec3& pos = c.LocalPosition;
			j["Position"] = { rf(pos.x), rf(pos.y), rf(pos.z) };
		}
		if (IsRotationAndScaleSerialized(isPrefabEntity))
		{
			j["Rotation"] = rf(c.Rotation);
			j["Scale"] = { rf(c.Scale.x), rf(c.Scale.y) };
		}
	}

	template<>
	void SceneSerializer::Deserialize<TransformComponent>(Entity entity, TransformComponent& c, const json& j)
	{
		const bool isPrefabEntity = entity.HasComponent<PrefabComponent>();

		if (IsPositionSerialized())
		{
			const json& pos = j["Position"];
			c.WorldPosition = { pos[0], pos[1], pos[2] };
			c.LocalPosition = { pos[0], pos[1], pos[2] };
		}
		if (IsRotationAndScaleSerialized(isPrefabEntity))
		{
			const json& scale = j["Scale"];
			const json& rotation = j["Rotation"];
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
			j["Texture"] = GetFilepathRelative("content/textures/", sprite.GetTexture()->GetPath());
			j["FilterMode"] = sprite.GetTexture()->GetFilterMode();
			j["Flip"] = { sprite.m_MirrorFlip.x, sprite.m_MirrorFlip.y };

			if (sprite.m_Spritesheet)
			{
				j["TilePos"] = { sprite.m_TilePos.x, sprite.m_TilePos.y };
				j["TileSize"] = { sprite.m_TileSize.x, sprite.m_TileSize.y };
			}
		}
		j["Color"] = { rf(color.r), rf(color.g), rf(color.b), rf(color.a) };
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
			j["Spritesheet"] = GetFilepathRelative("content/textures/", spritesheet->GetTexture()->GetPath());
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

	bool SceneSerializer::SerializeToFile(const std::string& filepath)
	{
		std::string output = SerializeScene();
		if (output.size() == 0)
			return false;

		std::ofstream out(filepath);
		out << output;
		out.close();
		return true;
	}

	bool SceneSerializer::DeserializeFromFile(const std::string& filepath)
	{
		std::string jsonData = Utils::ReadFile(filepath);
		if (jsonData.size())
			return DeserializeScene(jsonData);
		return false;
	}

	std::string SceneSerializer::SerializeEntityToString(Entity entity)
	{
		return SerializeEntity(entity).dump();
	}

	template<typename TComponent>
	void SceneSerializer::Serialize(Entity entity, const TComponent& c, json& j)
	{
		static_assert(sizeof(TComponent) == 0); // missing method definition
	}

	template<typename TComponent>
	void SceneSerializer::Deserialize(Entity entity, TComponent& c, const json& j)
	{
		static_assert(sizeof(TComponent) == 0); // missing method definition
	}

	template<typename TComponent>
	inline void SceneSerializer::TrySerialize(std::string_view key, Entity entity, json& j)
	{
		if (entity.HasComponent<TComponent>())
		{
			const auto& component = entity.GetComponent<TComponent>();
			SceneSerializer::Serialize<TComponent>(entity, component, j[key]);

			if (j[key].is_null())
				j.erase(key);
		}
	}

	template<typename TComponent>
	inline void SceneSerializer::TryDeserialize(std::string_view key, Entity entity, const json& j)
	{
		if (j.contains(key))
		{
			TComponent* component = entity.HasComponent<TComponent>() ?
				&entity.GetComponent<TComponent>() : &entity.AddComponent<TComponent>();

			SceneSerializer::Deserialize<TComponent>(entity, *component, j[key]);
		}
	}

}
