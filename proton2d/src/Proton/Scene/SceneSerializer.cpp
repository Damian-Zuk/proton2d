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

	static const std::string s_TexturesPath = "content/textures/";

	static std::string GetFilepathRelative(const std::string& dir, const std::string& filepath)
	{
		return filepath.substr(dir.size(), filepath.size() - dir.size());;
	}

	static inline double round(float f)
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
		const auto& col = m_Scene->m_ClearColor;

		// Serialize scene properties
		json data = {
			{ "GameModeClass",      m_Scene->m_GameModeClassName },
			{ "EnableNetworking",   m_Scene->m_EnableNetworking },
			{ "EnablePhysics",      m_Scene->m_EnablePhysics },
			{ "GravityForce",       m_Scene->m_PhysicsWorld->m_Gravity },
			{ "VelocityIterations", m_Scene->m_PhysicsWorld->m_PhysicsVelocityIterations },
			{ "PositionIterations", m_Scene->m_PhysicsWorld->m_PhysicsPositionIterations },
			{ "ScreenClearColor", { col.r, col.g, col.b, col.a } }
		};

		Entity primaryCameraEntity = m_Scene->GetPrimaryCameraEntity();
		if (primaryCameraEntity.IsValid())
		{
			uint64_t id = m_Scene->GetPrimaryCameraEntity().GetUUID();
			data["PrimaryCameraEntity"] = id;
		}

		// Serialize entities
		for (Entity entity : m_Scene->m_Root)
		{
			m_IsRootEntity = true;
			data["Entities"].push_back(SerializeEntity(entity));
		}

		// Reset state
		m_IsRootEntity = true;
		m_IsParentPrefab = false;

		return data.dump(4);
	}

	bool SceneSerializer::DeserializeScene(const std::string& jsonData)
	{
		PT_CORE_VERIFY(m_Scene, "Invalid Scene");
		json data = json::parse(jsonData);

		// Deserialize scene properties
		m_Scene->SetGameModeByClassName(data["GameModeClass"]);
		m_Scene->m_EnablePhysics = data["EnablePhysics"];
		m_Scene->m_EnableNetworking = data["EnableNetworking"];
		m_Scene->m_PhysicsWorld->m_Gravity = data["GravityForce"];
		m_Scene->m_PhysicsWorld->m_PhysicsVelocityIterations = data["VelocityIterations"];
		m_Scene->m_PhysicsWorld->m_PhysicsPositionIterations = data["PositionIterations"];
		
		const json& col = data["ScreenClearColor"];
		m_Scene->m_ClearColor = { col[0], col[1], col[2], col[3] };

		// Deserialize scene entities
		const json& entities = data["Entities"];
		for (auto it = entities.begin(); it != entities.end(); it++)
		{
			m_IsRootEntity = true;
			DeserializeEntity(*it);
		}

		// Set primary camera entity
		if (data.contains("PrimaryCameraEntity"))
		{
			UUID id{ data["PrimaryCameraEntity"] };
			m_Scene->SetPrimaryCameraEntity(m_Scene->FindByID(id));
		}

		m_Scene->CalculateWorldPositions();

		// Reset state
		m_IsRootEntity = true;
		m_IsParentPrefab = false;

		return true;
	}

	json SceneSerializer::SerializeEntity(Entity entity)
	{
		json out;

		bool isPrefabEntity = entity.HasComponent<PrefabComponent>();
		uint64_t prefabUUID = isPrefabEntity ? entity.GetComponent<PrefabComponent>().PrefabUUID : 0;

		// Serialize TagComponent
		auto& tag = entity.GetTag();
		out["Tag"] = tag;

		// Serialize IDComponent
		auto& id = entity.GetComponent<IDComponent>();
		if (Format == FormatType::Scene)
		{
			out["UUID"] = (uint64_t)id.ID;
			if (m_IsParentPrefab)
			{
				out["PrefabChildRefUUID"] = (uint64_t)id.PrefabChildRefID;
			}
		}
		else if (Format == FormatType::Prefab)
		{
			if (isPrefabEntity && m_IsRootEntity)
			{
				out["UUID"] = prefabUUID;
			}
			else
				out["UUID"] = (uint64_t)id.PrefabChildRefID;
		}

		// Serialize PrefabComponent
		if (isPrefabEntity)
		{
			out["PrefabUUID"] = prefabUUID;
		}

		// Serialize TransformComponent
		const auto& transform = entity.GetTransform();

		if ((Format == FormatType::Scene && !m_IsParentPrefab) ||
			(Format == FormatType::Prefab && m_IsParentPrefab))
		{
			const auto& position = transform.LocalPosition;
			out["Transform"]["Position"] = { round(position.x), round(position.y), round(position.z) };
		}

		if ((Format == FormatType::Scene && !m_IsParentPrefab) ||
			(Format == FormatType::Prefab && (!m_IsParentPrefab || !isPrefabEntity)))
		{
			out["Transform"]["Rotation"] = round(transform.Rotation);
			out["Transform"]["Scale"] = { round(transform.Scale.x), round(transform.Scale.y) };
		}

		// Check if there is need to serialize all entity components for this format
		if ((Format == FormatType::Scene && (isPrefabEntity || m_IsParentPrefab)) ||
			(Format == FormatType::Prefab && m_IsParentPrefab && isPrefabEntity))
		{
			SerializeChildren(entity, out);
			return out;
		}

		#define _TrySerialize(TComponent) \
			TrySerialize<TComponent>(#TComponent, entity, out)

		_TrySerialize(NetworkComponent);
		_TrySerialize(SpriteComponent);
		_TrySerialize(ResizableSpriteComponent);
		_TrySerialize(CircleRendererComponent);
		_TrySerialize(TextComponent);
		_TrySerialize(CameraComponent);
		_TrySerialize(RigidbodyComponent);
		_TrySerialize(BoxColliderComponent);
		_TrySerialize(CircleColliderComponent);
		_TrySerialize(ScriptComponent);

		// Serialize child entities
		SerializeChildren(entity, out);
		return out;
	}

	void SceneSerializer::SerializeChildren(Entity entity, json& out)
	{
		bool prev = m_IsParentPrefab;
		m_IsParentPrefab |= entity.HasComponent<PrefabComponent>();

		auto& relationship = entity.GetComponent<RelationshipComponent>();
		if (relationship.ChildrenCount)
		{
			entt::entity current = relationship.First;
			while (current != entt::null)
			{
				Entity child{ current, entity.m_Scene };
				auto& rc = child.GetComponent<RelationshipComponent>();
				m_IsRootEntity = false;
				out["Entities"].push_back(SerializeEntity(child));
				current = rc.Next;
			}
		}
		m_IsParentPrefab = prev;
	}

	Entity SceneSerializer::DeserializeEntity(const json& data, Entity entity)
	{
		if (!entity) // Create new entity if did not provided existing entity
		{
			UUID uuid = (Format == FormatType::Scene && data.contains("UUID")) ? (uint64_t)data["UUID"] : UUID();
			entity = m_Scene->CreateEntityWithUUID(uuid);
		}

		if (Format == FormatType::Scene && data.contains("PrefabChildRefUUID"))
		{
			entity.GetComponent<IDComponent>().PrefabChildRefID = (uint64_t)data["PrefabChildRefUUID"];
		}

		// Deserialize TagComponent
		entity.GetComponent<TagComponent>().Tag = data["Tag"];

		bool isPrefabEntity = data.contains("PrefabUUID");

		// Deserialize TransformComponent
		auto& transform = entity.GetComponent<TransformComponent>();

		if ((Format == FormatType::Scene && !m_IsParentPrefab) ||
			(Format == FormatType::Prefab && m_IsParentPrefab))
		{
			const json& position = data["Transform"]["Position"];
			transform.WorldPosition = { position[0], position[1], position[2] };
			transform.LocalPosition = { position[0], position[1], position[2] };
		}

		if ((Format == FormatType::Scene && !m_IsParentPrefab) ||
			(Format == FormatType::Prefab && (!m_IsParentPrefab || !isPrefabEntity)))
		{
			const json& scale = data["Transform"]["Scale"];
			const json& rotation = data["Transform"]["Rotation"];
			transform.Scale = { scale[0], scale[1] };
			transform.Rotation = rotation;
		}

		// Deserialize PrefabComponent
		if (isPrefabEntity)
		{
			auto& pc = entity.AddOrReplaceComponent<PrefabComponent>();
			pc.PrefabUUID = (uint64_t)data["PrefabUUID"];

			if (Format == FormatType::Scene || m_IsParentPrefab)
			{
				DeserializeChildren(entity, data);
				PrefabManager::DeserializePrefab(entity, pc.PrefabUUID);
				return entity;
			}
		}

		#define _TryDeserialize(TComponent) \
			TryDeserialize<TComponent>(#TComponent, entity, data)

		_TryDeserialize(NetworkComponent);
		_TryDeserialize(SpriteComponent);
		_TryDeserialize(ResizableSpriteComponent);
		_TryDeserialize(CircleRendererComponent);
		_TryDeserialize(TextComponent);
		_TryDeserialize(CameraComponent);
		_TryDeserialize(RigidbodyComponent);
		_TryDeserialize(BoxColliderComponent);
		_TryDeserialize(CircleColliderComponent);
		_TryDeserialize(ScriptComponent);

		DeserializeChildren(entity, data);
		return entity;
	}

	void SceneSerializer::DeserializeChildren(Entity entity, const json& data)
	{
		if (!data.contains("Entities"))
			return;

		bool restoreValue = m_IsParentPrefab;
		m_IsParentPrefab |= entity.HasComponent<PrefabComponent>();
		m_IsRootEntity = false;

		auto& hierarchy = entity.GetComponent<RelationshipComponent>();
		const json& entities = data["Entities"];

		if (hierarchy.ChildrenCount == 0)
		{
			// Create new child entities
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
		else if (Format == FormatType::Prefab)
		{
			// Create mapping for existing hierarchy
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
		m_IsParentPrefab = restoreValue;
	}

	template<>
	void SceneSerializer::Serialize<SpriteComponent>(Entity entity, const SpriteComponent& c, json& out)
	{
		auto& sprite = c.Sprite;
		auto& color = c.Color;
		if (sprite)
		{
			out = {
				{ "Texture", GetFilepathRelative(s_TexturesPath, sprite.GetTexture()->GetPath())},
				{ "FilterMode", sprite.GetTexture()->GetFilterMode() },
				{ "Flip", { sprite.m_MirrorFlip.x, sprite.m_MirrorFlip.y } }
			};

			if (sprite.m_Spritesheet)
			{
				out["TilePos"] = { sprite.m_TilePos.x, sprite.m_TilePos.y };
				out["TileSize"] = { sprite.m_TileSize.x, sprite.m_TileSize.y };
			}
		}
		out["Color"] = { round(color.r), round(color.g), round(color.b), round(color.a) };
	}

	template<>
	void SceneSerializer::Deserialize<SpriteComponent>(Entity entity, SpriteComponent& c, const json& j)
	{
		const json& col = j["Color"];
		c.Color = { col[0], col[1], col[2], col[3] };

		if (!j.contains("Texture"))
			return;

		const auto& texture = AssetManager::GetTexture(j["Texture"]);
		if (!texture)
		{
			PT_CORE_ERROR("Texture '{}' does not exist!", j["Texture"]);
			return;
		}

		texture->m_FilterMode = j["FilterMode"];
		c.Sprite.SetTexture(texture);
		c.Sprite.m_MirrorFlip.x = j["Flip"][0];
		c.Sprite.m_MirrorFlip.y = j["Flip"][1];

		if (!j.contains("TilePos"))
			return;

		const auto& spritesheet = AssetManager::GetSpritesheet(j["Texture"]);
		if (!spritesheet)
		{
			PT_CORE_ERROR("Spritesheet {} does not exist!", j["Texture"]);
			return;
		}

		c.Sprite.SetSpritesheet(spritesheet);
		c.Sprite.SetTile(j["TilePos"][0], j["TilePos"][1]);
		c.Sprite.SetTileSize(j["TileSize"][0], j["TileSize"][1]);
	}

	template<>
	void SceneSerializer::Serialize<ResizableSpriteComponent>(Entity entity, const ResizableSpriteComponent& c, json& out)
	{
		auto& sprite = c.ResizableSprite;
		auto& spritesheet = sprite.GetSpritesheet();
		const auto& col = c.Color;

		out = {
			{ "Width",     sprite.m_CellCount.x },
			{ "Height",    sprite.m_CellCount.y },
			{ "TileScale", sprite.m_CellScale },
			{ "Edges",     sprite.GetEdgesBitset() },
			{ "Offset",    { sprite.m_PatternOffset.x, sprite.m_PatternOffset.y } },
			{ "PatternSize", { sprite.m_PatternSize.x, sprite.m_PatternSize.y }},
			{ "Color", { col.r, col.g, col.b, col.a } }
		};

		if (spritesheet)
			out["Spritesheet"] = GetFilepathRelative(s_TexturesPath, spritesheet->GetTexture()->GetPath());
	}

	template<>
	void SceneSerializer::Deserialize<ResizableSpriteComponent>(Entity entity, ResizableSpriteComponent& c, const json& j)
	{
		auto& sprite = c.ResizableSprite;
		sprite.m_EdgesBitset = j["Edges"];
		sprite.m_CellScale = j["TileScale"];

		if (j.contains("Offset"))
			sprite.m_PatternOffset = { j.at("Offset")[0], j.at("Offset")[1] };

		if (j.contains("PatternSize"))
			sprite.m_PatternSize = { j.at("PatternSize")[0], j.at("PatternSize")[1] };

		if (j.contains("Spritesheet"))
			sprite.m_Spritesheet = AssetManager::GetSpritesheet(j["Spritesheet"]);

		auto& transform = entity.GetComponent<TransformComponent>();
		sprite.Generate(transform.Scale);

		const json& col = j["Color"];
		c.Color = { col[0], col[1], col[2], col[3] };
	}

	template<>
	void SceneSerializer::Serialize<CircleRendererComponent>(Entity entity, const CircleRendererComponent& c, json& out)
	{
		const auto& col = c.Color;
		out = {
			{ "Thickness", c.Thickness },
			{ "Fade",      c.Fade },
			{ "Color", { col.r, col.g, col.b, col.a } }
		};
	}

	template<>
	void SceneSerializer::Deserialize<CircleRendererComponent>(Entity entity, CircleRendererComponent& c, const json& j)
	{
		c.Thickness = j["Thickness"];
		c.Fade = j["Fade"];

		const json& col = j["Color"];
		c.Color = { col[0], col[1], col[2], col[3] };
	}

	template<>
	void SceneSerializer::Serialize<TextComponent>(Entity entity, const TextComponent& c, json& out)
	{
		const auto& col = c.Color;
		out = {
			{ "TextString",  c.TextString },
			{ "Kerning",     c.Kerning },
			{ "LineSpacing", c.LineSpacing },
			{ "Color", { col.r, col.g, col.b, col.a } },
			{ "Hidden", c.Hidden }
		};
	}

	template<>
	void SceneSerializer::Deserialize<TextComponent>(Entity entity, TextComponent& c, const json& j)
	{
		c.TextString = j["TextString"];
		c.Kerning = j["Kerning"];
		c.LineSpacing = j["LineSpacing"];
		c.Hidden = j["Hidden"];

		const json& col = j["Color"];
		c.Color = { col[0], col[1], col[2], col[3] };
	}
	
	template<>
	void SceneSerializer::Serialize<CameraComponent>(Entity entity, const CameraComponent& c, json& out)
	{
		out = {
			{ "ZoomLevel", c.Camera.GetZoomLevel() },
			{ "PositionOffset", { c.PositionOffset.x, c.PositionOffset.y } },
		};
	}

	template<>
	void SceneSerializer::Deserialize<CameraComponent>(Entity entity, CameraComponent& c, const json& j)
	{
		c.Camera.SetZoomLevel(j["ZoomLevel"]);
		c.PositionOffset = { j["PositionOffset"][0], j["PositionOffset"][1] };
	}

	template<>
	void SceneSerializer::Serialize<RigidbodyComponent>(Entity entity, const RigidbodyComponent& c, json& out)
	{
		out = {
			{ "Type", c.Type },
			{ "LinearDamping", c.LinearDamping },
			{ "AngularDamping", c.AngularDamping },
			{ "GravityScale", c.GravityScale },
			{ "IsBullet", c.IsBullet },
			{ "FixedRotation", c.FixedRotation },
			{ "AttachToParent", c.AttachToParent }
		};
	}

	template<>
	void SceneSerializer::Deserialize<RigidbodyComponent>(Entity entity, RigidbodyComponent& c, const json& j)
	{
		if (j.contains("Type"))
			c.Type = j["Type"];

		if (j.contains("LinearDamping"))
			c.LinearDamping = j["LinearDamping"];

		if (j.contains("AngularDamping"))
			c.AngularDamping = j["AngularDamping"];

		if (j.contains("GravityScale"))
			c.GravityScale = j["GravityScale"];

		if (j.contains("IsBullet"))
			c.IsBullet = j["IsBullet"];

		if (j.contains("FixedRotation"))
			c.FixedRotation = j["FixedRotation"];

		if (j.contains("AttachToParent"))
			c.AttachToParent = j["AttachToParent"];
	}

	template<>
	void SceneSerializer::Serialize<BoxColliderComponent>(Entity entity, const BoxColliderComponent& c, json& out)
	{
		out = {
			{ "Size", { c.Size.x,   c.Size.y } },
			{ "Offset", { c.Offset.x, c.Offset.y } },
			{ "Friction", c.Material.Friction },
			{ "Restitution", c.Material.Restitution },
			{ "RestitutionThreshold", c.Material.RestitutionThreshold },
			{ "Density", c.Material.Density },
			{ "IsSensor", c.IsSensor },
			{ "AttachToParent",c.AttachToParent }
		};
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

		if (j.contains("AttachToParent"))
			c.AttachToParent = j.at("AttachToParent");
	}

	template<>
	void SceneSerializer::Serialize<CircleColliderComponent>(Entity entity, const CircleColliderComponent& c, json& out)
	{
		out = {
			{ "Offset", { c.Offset.x, c.Offset.y } },
			{ "Radius", c.Radius },
			{ "Friction", c.Material.Friction },
			{ "Restitution", c.Material.Restitution },
			{ "RestitutionThreshold", c.Material.RestitutionThreshold },
			{ "Density", c.Material.Density },
			{ "IsSensor", c.IsSensor },
			{ "AttachToParent", c.AttachToParent }
		};
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

		if (j.contains("AttachToParent"))
			c.AttachToParent = j.at("AttachToParent");
	}

	template<>
	void SceneSerializer::Serialize<NetworkComponent>(Entity entity, const NetworkComponent& c, json& out)
	{
		auto& netTransform = c.NetTransform;
		out = {
			{ "SimulateOnClient", c.SimulateOnClient },
			{ "SyncMethod", NetSyncMethodToString(netTransform.Method) },
			{ "CullDistance", netTransform.CullDistance, },
			{ "TeleportThreshold", netTransform.TeleportThreshold },
			{ "ReconcileThreshold", netTransform.ReconcileThreshold },
			{ "ReconcileMaxTime", netTransform.ReconcileMaxTime },
		};
	}

	template<>
	void SceneSerializer::Deserialize<NetworkComponent>(Entity entity, NetworkComponent& c, const json& j)
	{
		auto& netTransform = c.NetTransform;

		if (j.contains("SimulateOnClient"))
			c.SimulateOnClient = j["SimulateOnClient"];

		if (j.contains("SyncMethod"))
			netTransform.Method = StringToNetSyncMethod(j["SyncMethod"]);

		if (j.contains("CullDistance"))
			netTransform.CullDistance = j["CullDistance"];

		if (j.contains("TeleportThreshold"))
			netTransform.TeleportThreshold = j["TeleportThreshold"];

		if (j.contains("ReconcileThreshold"))
			netTransform.ReconcileThreshold = j["ReconcileThreshold"];

		if (j.contains("ReconcileMaxTime"))
			netTransform.ReconcileMaxTime = j["ReconcileMaxTime"];
	}

	template<>
	void SceneSerializer::Serialize<ScriptComponent>(Entity entity, const ScriptComponent& c, json& out)
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

			out.push_back(scriptJ);
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
		if (output.size() > 0)
		{
			std::ofstream out(filepath);
			out << output;
			out.close();
			return true;
		}
		return false;
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
	inline void SceneSerializer::TrySerialize(std::string_view key, Entity entity, json& out)
	{
		if (entity.HasComponent<TComponent>())
		{
			const auto& component = entity.GetComponent<TComponent>();
			SceneSerializer::Serialize<TComponent>(entity, component, out[key]);
		}
	}

	template<typename TComponent>
	inline void SceneSerializer::TryDeserialize(std::string_view key, Entity entity, const json& data)
	{
		if (data.contains(key))
		{
			auto& component = entity.AddOrReplaceComponent<TComponent>();
			SceneSerializer::Deserialize<TComponent>(entity, component, data[key]);
		}
	}

}
