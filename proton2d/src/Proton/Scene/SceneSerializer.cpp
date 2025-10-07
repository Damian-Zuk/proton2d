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

	std::string SceneSerializer::Serialize()
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

		return data.dump(4);
	}

	bool SceneSerializer::Deserialize(const std::string& jsonData)
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
		return true;
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

	json SceneSerializer::SerializeEntity(Entity entity)
	{
		json out;

		/**************************************| Serialize Metadata |**************************************/  

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

		/**************************************| Serialize Components |**************************************/

		// Serialize NetworkComponent
		if (entity.HasComponent<NetworkComponent>())
		{
			auto& net = entity.GetComponent<NetworkComponent>();
			auto& netTransform = net.NetTransform;
			
			out["Network"] = {
				{ "SimulateOnClient", net.SimulateOnClient },
				{ "SyncMethod", NetSyncMethodToString(netTransform.Method) },
				{ "CullDistance", netTransform.CullDistance, },
				{ "TeleportThreshold", netTransform.TeleportThreshold },
				{ "ReconcileThreshold", netTransform.ReconcileThreshold },
				{ "ReconcileMaxTime", netTransform.ReconcileMaxTime },
			};
		}

		// Serialize SpriteComponent
		if (entity.HasComponent<SpriteComponent>())
		{
			auto& spriteComponent = entity.GetComponent<SpriteComponent>();
			auto& sprite = spriteComponent.Sprite;
			auto& color = spriteComponent.Color;
			if (sprite)
			{
				out["Sprite"] = {
					{ "Texture", GetFilepathRelative(s_TexturesPath, sprite.GetTexture()->GetPath())},
					{ "FilterMode", sprite.GetTexture()->GetFilterMode() },
					{ "Flip", { sprite.m_MirrorFlip.x, sprite.m_MirrorFlip.y } }
				};

				if (sprite.m_Spritesheet)
				{
					out["Sprite"]["TilePos"] = { sprite.m_TilePos.x, sprite.m_TilePos.y }; 
					out["Sprite"]["TileSize"] = { sprite.m_TileSize.x, sprite.m_TileSize.y };
				}
			}
			out["Sprite"]["Color"] = { round(color.r), round(color.g), round(color.b), round(color.a) };
		}
		
		// Serialize ResizableSpriteComponent
		if (entity.HasComponent<ResizableSpriteComponent>())
		{
			auto& component = entity.GetComponent<ResizableSpriteComponent>();
			auto& sprite = component.ResizableSprite;
			auto& spritesheet = sprite.GetSpritesheet();
			const auto& col = component.Color;

			out["ResizableSprite"] = {
				{ "Width",     sprite.m_CellCount.x },
				{ "Height",    sprite.m_CellCount.y },
				{ "TileScale", sprite.m_CellScale },
				{ "Edges",     sprite.GetEdgesBitset() },
				{ "Offset",    { sprite.m_PatternOffset.x, sprite.m_PatternOffset.y } },
				{ "PatternSize", { sprite.m_PatternSize.x, sprite.m_PatternSize.y }},
				{ "Color", { col.r, col.g, col.b, col.a } }
			};

			if (spritesheet)
			{
				out["ResizableSprite"]["Spritesheet"] = GetFilepathRelative(s_TexturesPath, spritesheet->GetTexture()->GetPath());
			}
		}

		// Serialize CircleRendererComponent
		if (entity.HasComponent<CircleRendererComponent>())
		{
			auto& component = entity.GetComponent<CircleRendererComponent>();
			const auto& col = component.Color;

			out["CircleRenderer"] = {
				{ "Thickness", component.Thickness },
				{ "Fade",      component.Fade },
				{ "Color", { col.r, col.g, col.b, col.a } }
			};
		}

		// Serialize TextCompontent
		if (entity.HasComponent<TextComponent>())
		{
			auto& component = entity.GetComponent<TextComponent>();
			const auto& col = component.Color;

			out["Text"] = {
				{ "TextString",  component.TextString },
				{ "Kerning",     component.Kerning },
				{ "LineSpacing", component.LineSpacing },
				{ "Color", { col.r, col.g, col.b, col.a } },
				{ "Hidden", component.Hidden }
			};
		}

		// Serialize RigidbodyComponent
		if (entity.HasComponent<RigidbodyComponent>())
		{
			auto& rb = entity.GetComponent<RigidbodyComponent>();
			out["Rigidbody"] = {
				{ "Type", rb.Type },
				{ "LinearDamping", rb.LinearDamping },
				{ "AngularDamping", rb.AngularDamping },
				{ "GravityScale", rb.GravityScale },
				{ "IsBullet", rb.IsBullet },
				{ "FixedRotation", rb.FixedRotation },
				{ "AttachToParent", rb.AttachToParent }
			};
		}

		// Serialize BoxColliderComponent
		if (entity.HasComponent<BoxColliderComponent>())
		{
			auto& collider = entity.GetComponent<BoxColliderComponent>();
			out["BoxCollider"] = {
				{ "Size",               { collider.Size.x,   collider.Size.y } },
				{ "Offset",             { collider.Offset.x, collider.Offset.y } },
				{ "Friction",             collider.Material.Friction },
				{ "Restitution",          collider.Material.Restitution },
				{ "RestitutionThreshold", collider.Material.RestitutionThreshold },
				{ "Density",              collider.Material.Density },
				{ "IsSensor",             collider.IsSensor },
				{ "AttachToParent",       collider.AttachToParent }
			};
		}

		// Serialize CircleColliderComponent
		if (entity.HasComponent<CircleColliderComponent>())
		{
			auto& collider = entity.GetComponent<CircleColliderComponent>();
			out["CircleCollider"] = {
				{ "Offset",             { collider.Offset.x, collider.Offset.y } },
				{ "Radius",               collider.Radius },
				{ "Friction",             collider.Material.Friction },
				{ "Restitution",          collider.Material.Restitution },
				{ "RestitutionThreshold", collider.Material.RestitutionThreshold },
				{ "Density",              collider.Material.Density },
				{ "IsSensor",             collider.IsSensor },
				{ "AttachToParent",       collider.AttachToParent }
			};
		}

		// Serialize CameraComponent
		if (entity.HasComponent<CameraComponent>())
		{
			auto& camera = entity.GetComponent<CameraComponent>();
			out["Camera"] = {
				{ "ZoomLevel", camera.Camera.GetZoomLevel() },
				{ "PositionOffset", { camera.PositionOffset.x, camera.PositionOffset.y } },
			};
		}

		// Serialize ScriptComponent
		if (entity.HasComponent<ScriptComponent>())
		{
			auto& script = entity.GetComponent<ScriptComponent>();
			for (auto& [scriptClassName, scriptInstance] : script.Scripts)
			{
				json scriptObj;
				scriptObj["ClassName"] = scriptClassName;
				for (auto& [fieldName, fieldData] : scriptInstance->m_ScriptFields)
				{
					json fieldObj;
					fieldObj["FieldName"] = fieldName;

					switch (fieldData.Type)
					{
					case ScriptFieldType::Float:
						fieldObj["Value"] = *(float*)fieldData.ValuePtr;
						break;
					case ScriptFieldType::Float2:
					{
						const glm::vec2& data = *(glm::vec2*)fieldData.ValuePtr;
						fieldObj["Value"] = { data.x, data.y };
						break;
					}
					case ScriptFieldType::Float3:
					{
						const glm::vec3& data = *(glm::vec3*)fieldData.ValuePtr;
						fieldObj["Value"] = { data.x, data.y, data.z };
						break;
					}
					case ScriptFieldType::Float4:
					{
						const glm::vec4& data = *(glm::vec4*)fieldData.ValuePtr;
						fieldObj["Value"] = { data.x, data.y, data.z, data.a };
						break;
					}

					case ScriptFieldType::Int:
						fieldObj["Value"] = *(int*)fieldData.ValuePtr;
						break;
					case ScriptFieldType::Int2:
					{
						const glm::ivec2& data = *(glm::ivec2*)fieldData.ValuePtr;
						fieldObj["Value"] = { data.x, data.y };
						break;
					}
					case ScriptFieldType::Int3:
					{
						const glm::ivec3& data = *(glm::ivec3*)fieldData.ValuePtr;
						fieldObj["Value"] = { data.x, data.y, data.z };
						break;
					}
					case ScriptFieldType::Int4:
					{
						const glm::ivec4& data = *(glm::ivec4*)fieldData.ValuePtr;
						fieldObj["Value"] = { data.x, data.y, data.z, data.a };
						break;
					}

					case ScriptFieldType::Bool:
						fieldObj["Value"] = *(bool*)fieldData.ValuePtr;
						break;

					case ScriptFieldType::String:
						fieldObj["Value"] = *(std::string*)fieldData.ValuePtr;
						break;
					}
					scriptObj["Fields"].push_back(fieldObj);
				}
				out["Scripts"].push_back(scriptObj);
			}
		}

		// Serialize child entities
		SerializeChildren(entity, out);
		return out;
	}

	// *****************************************
	//       Deserialize Entity Function
	// *****************************************

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
					PT_CORE_VERIFY(data.contains("UUID"));
					child.GetComponent<IDComponent>().PrefabChildRefID = (uint64_t)data["UUID"];
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

				PT_CORE_VERIFY(entityData.contains(keyName));
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

		// Deserialize NetworkComponent
		if (data.contains("Network"))
		{
			auto& net = entity.AddOrReplaceComponent<NetworkComponent>();
			auto& netTransform = net.NetTransform;
			auto& netJson = data.at("Network");

			if (netJson.contains("SimulateOnClient"))
				net.SimulateOnClient = netJson["SimulateOnClient"];

			if (netJson.contains("SyncMethod"))
				netTransform.Method = StringToNetSyncMethod(netJson["SyncMethod"]);

			if (netJson.contains("CullDistance"))
				netTransform.CullDistance = netJson["CullDistance"];

			if (netJson.contains("TeleportThreshold"))
				netTransform.TeleportThreshold = netJson["TeleportThreshold"];

			if (netJson.contains("ReconcileThreshold"))
				netTransform.ReconcileThreshold = netJson["ReconcileThreshold"];

			if (netJson.contains("ReconcileMaxTime"))
				netTransform.ReconcileMaxTime = netJson["ReconcileMaxTime"];
		}

		// Deserialize SpriteComponent
		if (data.contains("Sprite"))
		{
			const json& sprite = data["Sprite"];
			auto& spriteComponent = entity.AddOrReplaceComponent<SpriteComponent>();
			
			if (sprite.contains("Texture"))
			{
				const auto& texture = AssetManager::GetTexture(sprite["Texture"]);
				if (texture)
				{
					if (sprite.contains("TilePos"))
					{
						const auto& spritesheet = AssetManager::GetSpritesheet(sprite["Texture"]);
						if (spritesheet)
						{
							spriteComponent.Sprite.SetSpritesheet(spritesheet);
							spriteComponent.Sprite.SetTile(sprite["TilePos"][0], sprite["TilePos"][1]);
							spriteComponent.Sprite.SetTileSize(sprite["TileSize"][0], sprite["TileSize"][1]);
						}
						else
							PT_CORE_ERROR("Spritesheet {} does not exist!", sprite["Texture"]);
					}
					else
						spriteComponent.Sprite.SetTexture(texture);

					spriteComponent.Sprite.GetTexture()->m_FilterMode = sprite["FilterMode"];
					spriteComponent.Sprite.m_MirrorFlip.x = sprite["Flip"][0];
					spriteComponent.Sprite.m_MirrorFlip.y = sprite["Flip"][1];
				}
				else
					PT_CORE_ERROR("Texture '{}' does not exist!", sprite["Texture"]);
			}
			
			const json& color = data["Sprite"]["Color"];
			spriteComponent.Color = { color[0], color[1], color[2], color[3] };
		}

		// Deserialize ResizableSpriteComponent
		if (data.contains("ResizableSprite"))
		{
			const json& jsonData = data["ResizableSprite"];
			auto& component = entity.AddOrReplaceComponent<ResizableSpriteComponent>();
			auto& sprite = component.ResizableSprite;
			sprite.m_EdgesBitset = jsonData["Edges"];
			sprite.m_CellScale = jsonData["TileScale"];

			if (jsonData.contains("Offset"))
				sprite.m_PatternOffset ={ jsonData.at("Offset")[0], jsonData.at("Offset")[1] };

			if (jsonData.contains("PatternSize"))
				sprite.m_PatternSize ={ jsonData.at("PatternSize")[0], jsonData.at("PatternSize")[1] };

			if (jsonData.contains("Spritesheet"))
				sprite.m_Spritesheet = AssetManager::GetSpritesheet(jsonData["Spritesheet"]);
		
			auto& transform = entity.GetComponent<TransformComponent>();
			sprite.Generate(transform.Scale);

			auto& c = jsonData["Color"];
			component.Color = { c[0], c[1], c[2], c[3] };
		}

		// Deserialize CircleRendererComponent
		if (data.contains("CircleRenderer"))
		{
			const json& jsonData = data["CircleRenderer"];
			auto& component = entity.AddOrReplaceComponent<CircleRendererComponent>();
			component.Thickness = jsonData["Thickness"];
			component.Fade = jsonData["Fade"];
			auto& c = jsonData["Color"];
			component.Color = { c[0], c[1], c[2], c[3] };
		}

		// Deserialize CameraComponent
		if (data.contains("Camera"))
		{
			auto& camera = entity.AddOrReplaceComponent<CameraComponent>();
			const json& cameraJson = data["Camera"];
			camera.Camera.SetZoomLevel(cameraJson["ZoomLevel"]);
			camera.PositionOffset = { cameraJson["PositionOffset"][0], cameraJson["PositionOffset"][1] };
		}

		// Deserialize BoxColliderComponent
		if (data.contains("BoxCollider"))
		{
			auto& collider = entity.AddOrReplaceComponent<BoxColliderComponent>();
			const json& boxCollider = data["BoxCollider"];
			const json& size = boxCollider["Size"];
			const json& offset = boxCollider["Offset"];

			collider.Size = { size[0], size[1] };
			collider.Offset = { offset[0], offset[1] };
			collider.Material.Friction = boxCollider["Friction"];
			collider.Material.Restitution = boxCollider["Restitution"];
			collider.Material.RestitutionThreshold = boxCollider["RestitutionThreshold"];
			collider.Material.Density = boxCollider["Density"];
			collider.IsSensor = boxCollider["IsSensor"];

			if (boxCollider.contains("AttachToParent"))
				collider.AttachToParent = boxCollider.at("AttachToParent");
		}

		// Deserialize CircleColliderComponent
		if (data.contains("CircleCollider"))
		{
			auto& collider = entity.AddOrReplaceComponent<CircleColliderComponent>();
			const json& circleCollider = data["CircleCollider"];
			const json& offset = circleCollider["Offset"];

			collider.Offset = { offset[0], offset[1] };
			collider.Radius = circleCollider["Radius"];
			collider.Material.Friction = circleCollider["Friction"];
			collider.Material.Restitution = circleCollider["Restitution"];
			collider.Material.RestitutionThreshold = circleCollider["RestitutionThreshold"];
			collider.Material.Density = circleCollider["Density"];
			collider.IsSensor = circleCollider["IsSensor"];

			if (circleCollider.contains("AttachToParent"))
				collider.AttachToParent = circleCollider.at("AttachToParent");
		}

		// Deserialize TextComponent
		if (data.contains("Text"))
		{
			auto& component = entity.AddOrReplaceComponent<TextComponent>();
			const json& jsonText = data["Text"];
			
			component.TextString = jsonText["TextString"];
			component.Kerning = jsonText["Kerning"];
			component.LineSpacing = jsonText["LineSpacing"];
			component.Hidden = jsonText["Hidden"];

			auto& c = jsonText["Color"];
			component.Color = { c[0], c[1], c[2], c[3] };
		}

		// Deserialize RigidbodyComponent
		if (data.contains("Rigidbody"))
		{
			auto& rb = entity.AddOrReplaceComponent<RigidbodyComponent>();
			const json& jsonRb = data["Rigidbody"];
			
			if (jsonRb.contains("Type"))
				rb.Type = jsonRb["Type"];

			if (jsonRb.contains("LinearDamping"))
				rb.LinearDamping = jsonRb["LinearDamping"];

			if (jsonRb.contains("AngularDamping"))
				rb.AngularDamping = jsonRb["AngularDamping"];

			if (jsonRb.contains("GravityScale"))
				rb.GravityScale = jsonRb["GravityScale"];

			if (jsonRb.contains("IsBullet"))
				rb.IsBullet = jsonRb["IsBullet"];

			if (jsonRb.contains("FixedRotation"))
				rb.FixedRotation = jsonRb["FixedRotation"];
			
			if (jsonRb.contains("AttachToParent"))
				rb.AttachToParent = jsonRb["AttachToParent"];
		}

		// Deserialize scripts
		if (data.contains("Scripts"))
		{
			for (auto& scriptJson : data["Scripts"])
			{
				std::string scriptClassName = scriptJson["ClassName"];
				EntityScript* script = ScriptFactory::Get().AddScriptToEntity(entity, scriptClassName);

				if (script == nullptr)
					continue;

				if (scriptJson.contains("Fields"))
				{
					const json& fields = scriptJson["Fields"];
					for (auto& field : fields)
					{
						std::string fieldName = field["FieldName"];
						if (script->m_ScriptFields.find(fieldName) != script->m_ScriptFields.end())
						{
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
			}
		}

		DeserializeChildren(entity, data);
		return entity;
	}

	bool SceneSerializer::DeserializeFromFile(const std::string& filepath)
	{
		std::string jsonData = Utils::ReadFile(filepath);
		if (jsonData.size())
			return Deserialize(jsonData);
		return false;
	}

	bool SceneSerializer::SerializeToFile(const std::string& filepath)
	{
		std::string output = Serialize();
		if (output.size() > 0)
		{
			std::ofstream out(filepath);
			out << output;
			out.close();
			return true;
		}
		return false;
	}

	std::string SceneSerializer::SerializeEntityToString(Entity entity)
	{
		return SerializeEntity(entity).dump();
	}

	void SceneSerializer::SetScene(Scene* scene)
	{
		m_Scene = scene;
	}

}
