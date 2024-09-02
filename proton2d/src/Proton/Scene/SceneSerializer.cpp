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

	static std::string GetFilepathRelative(const std::string& parentDir, const std::string& fullFilepath)
	{
		return fullFilepath.substr(parentDir.size(), fullFilepath.size() - parentDir.size());;
	}

	static inline double round(float f)
	{
		return std::round((double)f * 100000) / 100000;
	}

	SceneSerializer::SceneSerializer(Scene* scene)
		: m_Scene(scene)
	{
	}

	// *****************************************
	//         Serialize Scene Function
	// *****************************************

	bool SceneSerializer::SerializeToFile(const std::string& filepath)
	{
		std::ofstream out(filepath);
		out << Serialize();
		out.close();
		return true;
	}

	std::string SceneSerializer::Serialize()
	{
		PT_CORE_ASSERT(m_Scene, "Scene context not set!");
		const auto& c = m_Scene->m_ClearColor;
		json jsonObj = {
			{ "GameModeClass",      m_Scene->m_GameModeClassName },
			{ "EnablePhysics",      m_Scene->m_EnablePhysics },
			{ "GravityForce",       m_Scene->m_PhysicsWorld->m_Gravity },
			{ "VelocityIterations", m_Scene->m_PhysicsWorld->m_PhysicsVelocityIterations },
			{ "PositionIterations", m_Scene->m_PhysicsWorld->m_PhysicsPositionIterations },
			{ "ScreenClearColor", { c.r, c.g, c.b, c.a } },
			{ "EnableNetworking",     m_Scene->m_EnableNetworking }
		};

		Entity primaryCameraEntity = m_Scene->GetPrimaryCameraEntity();
		if (primaryCameraEntity.IsValid())
		{
			uint64_t id = m_Scene->GetPrimaryCameraEntity().GetUUID();
			jsonObj["PrimaryCameraEntity"] = id;
		}

		for (Entity entity : m_Scene->m_Root)
			jsonObj["Entities"].push_back(SerializeEntity(entity));

		return jsonObj.dump(4);
	}

	// *****************************************
	//       Deserialize Scene Function
	// *****************************************

	bool SceneSerializer::Deserialize(const std::string& jsonData)
	{
		json jsonObj = json::parse(jsonData);
		m_Scene->m_EnablePhysics = jsonObj["EnablePhysics"];
		
		if (jsonObj.contains("EnableNetworking"))
			m_Scene->m_EnableNetworking = jsonObj["EnableNetworking"];

		if (jsonObj.contains("GameModeClass"))
			m_Scene->SetGameModeByClassName(jsonObj["GameModeClass"]);

		m_Scene->m_PhysicsWorld->m_Gravity = jsonObj["GravityForce"];
		m_Scene->m_PhysicsWorld->m_PhysicsVelocityIterations = jsonObj["VelocityIterations"];
		m_Scene->m_PhysicsWorld->m_PhysicsPositionIterations = jsonObj["PositionIterations"];
		json& c = jsonObj["ScreenClearColor"];
		m_Scene->m_ClearColor = { c[0], c[1], c[2], c[3] };

		const json& entities = jsonObj["Entities"];
		for (auto it = entities.rbegin(); it != entities.rend(); it++)
			DeserializeEntity(*it);

		if (jsonObj.contains("PrimaryCameraEntity"))
		{
			UUID id{ jsonObj["PrimaryCameraEntity"] };
			m_Scene->SetPrimaryCameraEntity(m_Scene->FindByID(id));
		}
		m_Scene->CalculateWorldPositions();
		return true;
	}

	bool SceneSerializer::DeserializeFromFile(const std::string& filepath)
	{
		std::string jsonData = Utils::ReadFile(filepath);
		if (jsonData.size())
			return Deserialize(jsonData);
		return false;
	}

	// *****************************************
	//       Serialize Entity Function
	// *****************************************

	json SceneSerializer::SerializeEntity(Entity entity)
	{
		json jsonObj;

		bool isPrefab = entity.HasComponent<PrefabComponent>();

		// Serialize IDComponent
		auto& uuid = entity.GetUUID();
		jsonObj["UUID"] = (uint64_t)uuid;

		// Serialize TagComponent
		auto& tag = entity.GetComponent<TagComponent>().Tag;
		jsonObj["Tag"] = tag;

		// Serialize TransformComponent
		auto& transform = entity.GetComponent<TransformComponent>();
		auto& position = transform.LocalPosition;
		jsonObj["Transform"] = {
			{ "Position", { round(position.x), round(position.y), round(position.z) } },
			{ "Rotation", round(transform.Rotation) },
			{ "Scale", { round(transform.Scale.x), round(transform.Scale.y) } }
		};

		if (isPrefab)
		{
			auto& pc = entity.GetComponent<PrefabComponent>();
			jsonObj["Prefab"] = {
				{ "UUID", (uint64_t)pc.PrefabUUID },
			};

			if (!IsPrefabSerializer)
				return jsonObj;
		}

		// Serialize NetworkComponent
		if (entity.HasComponent<NetworkComponent>())
		{
			auto& net = entity.GetComponent<NetworkComponent>();
			auto& netTransform = net.NetTransform;
			
			jsonObj["Network"] = {
				{ "SimulateOnClient", net.SimulateOnClient },
				{ "SyncMethod", NetSyncMethodToString(netTransform.Method) },
				{ "CullDistance", netTransform.CullDistance, },
				{ "TeleportThreshold", netTransform.TeleportThreshold },
				{ "ReconcileThreshold", netTransform.ReconcileThreshold },
				{ "ReconcileMaxTime", netTransform.ReconcileMaxTime },
				{ "DeltaWeight", netTransform.DeltaWeight },
				{ "ServerVelocityWeight", netTransform.ServerVelocityWeight },
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
				jsonObj["Sprite"] = {
					{ "Texture", GetFilepathRelative(s_TexturesPath, sprite.GetTexture()->GetPath())},
					{ "FilterMode", sprite.GetTexture()->GetFilterMode() },
					{ "Flip", { sprite.m_MirrorFlip.x, sprite.m_MirrorFlip.y } }
				};

				if (sprite.m_Spritesheet)
				{
					jsonObj["Sprite"]["TilePos"] = { sprite.m_TilePos.x, sprite.m_TilePos.y }; 
					jsonObj["Sprite"]["TileSize"] = { sprite.m_TileSize.x, sprite.m_TileSize.y };
				}
			}
			jsonObj["Sprite"]["Color"] = { round(color.r), round(color.g), round(color.b), round(color.a) };
		}
		
		// Serialize ResizableSpriteComponent
		if (entity.HasComponent<ResizableSpriteComponent>())
		{
			auto& component = entity.GetComponent<ResizableSpriteComponent>();
			auto& sprite = component.ResizableSprite;
			auto& spritesheet = sprite.GetSpritesheet();
			const auto& col = component.Color;

			jsonObj["ResizableSprite"] = {
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
				jsonObj["ResizableSprite"]["Spritesheet"] = GetFilepathRelative(s_TexturesPath, spritesheet->GetTexture()->GetPath());
			}
		}

		// Serialize CircleRendererComponent
		if (entity.HasComponent<CircleRendererComponent>())
		{
			auto& component = entity.GetComponent<CircleRendererComponent>();
			const auto& col = component.Color;

			jsonObj["CircleRenderer"] = {
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

			jsonObj["Text"] = {
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
			jsonObj["Rigidbody"] = {
				{ "Type", rb.Type },
				{ "FixedRotation", rb.FixedRotation },
				{ "AttachToParent", rb.AttachToParent }
			};
		}

		// Serialize BoxColliderComponent
		if (entity.HasComponent<BoxColliderComponent>())
		{
			auto& collider = entity.GetComponent<BoxColliderComponent>();
			jsonObj["BoxCollider"] = {
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
			jsonObj["CircleCollider"] = {
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
			jsonObj["Camera"] = {
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
						fieldObj["Value"] = *(float*)fieldData.InstanceFieldValue;
						break;
					case ScriptFieldType::Float2:
					{
						const glm::vec2& data = *(glm::vec2*)fieldData.InstanceFieldValue;
						fieldObj["Value"] = { data.x, data.y };
						break;
					}
					case ScriptFieldType::Float3:
					{
						const glm::vec3& data = *(glm::vec3*)fieldData.InstanceFieldValue;
						fieldObj["Value"] = { data.x, data.y, data.z };
						break;
					}
					case ScriptFieldType::Float4:
					{
						const glm::vec4& data = *(glm::vec4*)fieldData.InstanceFieldValue;
						fieldObj["Value"] = { data.x, data.y, data.z, data.a };
						break;
					}

					case ScriptFieldType::Int:
						fieldObj["Value"] = *(int*)fieldData.InstanceFieldValue;
						break;
					case ScriptFieldType::Int2:
					{
						const glm::ivec2& data = *(glm::ivec2*)fieldData.InstanceFieldValue;
						fieldObj["Value"] = { data.x, data.y };
						break;
					}
					case ScriptFieldType::Int3:
					{
						const glm::ivec3& data = *(glm::ivec3*)fieldData.InstanceFieldValue;
						fieldObj["Value"] = { data.x, data.y, data.z };
						break;
					}
					case ScriptFieldType::Int4:
					{
						const glm::ivec4& data = *(glm::ivec4*)fieldData.InstanceFieldValue;
						fieldObj["Value"] = { data.x, data.y, data.z, data.a };
						break;
					}

					case ScriptFieldType::Bool:
						fieldObj["Value"] = *(bool*)fieldData.InstanceFieldValue;
						break;
					}
					scriptObj["Fields"].push_back(fieldObj);
				}
				jsonObj["Scripts"].push_back(scriptObj);
			}
		}

		// Serialize child entities
		auto& relationship = entity.GetComponent<RelationshipComponent>();
		if (relationship.ChildrenCount)
		{
			entt::entity current = relationship.First;
			while (current != entt::null)
			{
				Entity child{ current, entity.m_Scene };
				auto& rc = child.GetComponent<RelationshipComponent>();
				jsonObj["Entities"].push_back(SerializeEntity(child));
				current = rc.Next;
			}
		}

		return jsonObj;
	}

	std::string SceneSerializer::SerializeEntityToString(Entity entity)
	{
		return SerializeEntity(entity).dump();
	}

	// *****************************************
	//       Deserialize Entity Function
	// *****************************************

	Entity SceneSerializer::DeserializeEntity(const json& jsonObj, UUID uuid)
	{
		Entity entity;
		bool isPrefab = jsonObj.contains("Prefab");

		if (IsPrefabSerializer) // PrefabManager::Spawn
		{
			if (uuid)
				entity = m_Scene->CreateEntityWithUUID(uuid, jsonObj["Tag"]);
			else
				entity = m_Scene->CreateEntity(jsonObj["Tag"]); // New UUID
		}
		else
		{ 
			if (isPrefab)
			{
				entity = PrefabManager::Spawn(m_Scene, jsonObj["Tag"]);
				entity.GetComponent<IDComponent>().ID = (uint64_t)jsonObj["UUID"];
			}
			else
			{
				UUID uuidFromJson;
				if (jsonObj.contains("UUID"))
					uuidFromJson = (uint64_t)jsonObj["UUID"];
				entity = m_Scene->CreateEntityWithUUID(uuidFromJson, jsonObj["Tag"]);
			}
		}

		// Deserialize TransformComponent
		auto& transform = entity.GetComponent<TransformComponent>();
		const json& position = jsonObj["Transform"]["Position"];
		const json& scale    = jsonObj["Transform"]["Scale"];
		const json& rotation = jsonObj["Transform"]["Rotation"];
		transform.WorldPosition = { position[0], position[1], position[2] };
		transform.LocalPosition = { position[0], position[1], position[2] };
		transform.Scale    = { scale[0], scale[1] };
		transform.Rotation = rotation;

		// Deserialize PrefabComponent
		if (isPrefab)
		{
			auto& pc = entity.AddOrReplaceComponent<PrefabComponent>();
			pc.PrefabUUID = (uint64_t)jsonObj["Prefab"]["UUID"];

			if (!IsPrefabSerializer)
				// Other properties deserialized by PrefabManager::Spawn
				return entity;
		}

		// Deserialize NetworkComponent
		if (jsonObj.contains("Network"))
		{
			auto& net = entity.AddComponent<NetworkComponent>();
			auto& netTransform = net.NetTransform;
			auto& netJson = jsonObj.at("Network");

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

			if (netJson.contains("DeltaWeight"))
				netTransform.DeltaWeight = netJson["DeltaWeight"];

			if (netJson.contains("ServerVelocityWeight"))
				netTransform.ServerVelocityWeight = netJson["ServerVelocityWeight"];
		}

		// Deserialize SpriteComponent
		if (jsonObj.contains("Sprite"))
		{
			const json& sprite = jsonObj["Sprite"];
			auto& spriteComponent = entity.AddComponent<SpriteComponent>();
			
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
			
			const json& color = jsonObj["Sprite"]["Color"];
			spriteComponent.Color = { color[0], color[1], color[2], color[3] };
		}

		// Deserialize ResizableSpriteComponent
		if (jsonObj.contains("ResizableSprite"))
		{
			const json& jsonData = jsonObj["ResizableSprite"];
			auto& component = entity.AddComponent<ResizableSpriteComponent>();
			auto& sprite = component.ResizableSprite;
			sprite.m_EdgesBitset = jsonData["Edges"];
			sprite.m_CellScale = jsonData["TileScale"];

			if (jsonData.contains("Offset"))
				sprite.m_PatternOffset ={ jsonData.at("Offset")[0], jsonData.at("Offset")[1] };

			if (jsonData.contains("PatternSize"))
				sprite.m_PatternSize ={ jsonData.at("PatternSize")[0], jsonData.at("PatternSize")[1] };

			if (jsonData.contains("Spritesheet"))
				sprite.m_Spritesheet = AssetManager::GetSpritesheet(jsonData["Spritesheet"]);
		
			sprite.Generate(transform.Scale);

			auto& c = jsonData["Color"];
			component.Color = { c[0], c[1], c[2], c[3] };
		}

		// Deserialize CircleRendererComponent
		if (jsonObj.contains("CircleRenderer"))
		{
			const json& jsonData = jsonObj["CircleRenderer"];
			auto& component = entity.AddComponent<CircleRendererComponent>();
			component.Thickness = jsonData["Thickness"];
			component.Fade = jsonData["Fade"];
			auto& c = jsonData["Color"];
			component.Color = { c[0], c[1], c[2], c[3] };
		}

		// Deserialize CameraComponent
		if (jsonObj.contains("Camera"))
		{
			auto& camera = entity.AddComponent<CameraComponent>();
			const json& cameraJson = jsonObj["Camera"];
			camera.Camera.SetZoomLevel(cameraJson["ZoomLevel"]);
			camera.PositionOffset = { cameraJson["PositionOffset"][0], cameraJson["PositionOffset"][1] };
		}

		// Deserialize BoxColliderComponent
		if (jsonObj.contains("BoxCollider"))
		{
			auto& collider = entity.AddComponent<BoxColliderComponent>();
			const json& boxCollider = jsonObj["BoxCollider"];
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
		if (jsonObj.contains("CircleCollider"))
		{
			auto& collider = entity.AddComponent<CircleColliderComponent>();
			const json& circleCollider = jsonObj["CircleCollider"];
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
		if (jsonObj.contains("Text"))
		{
			auto& component = entity.AddComponent<TextComponent>();
			const json& jsonText = jsonObj["Text"];
			
			component.TextString = jsonText["TextString"];
			component.Kerning = jsonText["Kerning"];
			component.LineSpacing = jsonText["LineSpacing"];
			component.Hidden = jsonText["Hidden"];

			auto& c = jsonText["Color"];
			component.Color = { c[0], c[1], c[2], c[3] };
		}


		// Deserialize RigidbodyComponent
		if (jsonObj.contains("Rigidbody"))
		{
			auto& rb = entity.AddComponent<RigidbodyComponent>();
			const json& jsonRb = jsonObj.at("Rigidbody");
			rb.Type = jsonRb.at("Type");
			rb.FixedRotation = jsonRb.at("FixedRotation");
			
			if (jsonRb.contains("AttachToParent"))
				rb.AttachToParent = jsonRb.at("AttachToParent");
		}

		// Deserialize scripts
		if (jsonObj.contains("Scripts"))
		{
			for (auto& scriptJson : jsonObj["Scripts"])
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
								*(float*)scriptField.InstanceFieldValue = value;
								break;
							case ScriptFieldType::Float2:
								*(glm::vec2*)scriptField.InstanceFieldValue = { value[0], value[1] };
								break;											
							case ScriptFieldType::Float3:						
								*(glm::vec3*)scriptField.InstanceFieldValue = { value[0], value[1], value[2] };
								break;											
							case ScriptFieldType::Float4:						
								*(glm::vec4*)scriptField.InstanceFieldValue = { value[0], value[1], value[2], value[3] };
								break;

							case ScriptFieldType::Int:
								*(int*)scriptField.InstanceFieldValue = value;
								break;
							case ScriptFieldType::Int2:
								*(glm::ivec2*)scriptField.InstanceFieldValue = { value[0], value[1] };
								break;
							case ScriptFieldType::Int3:
								*(glm::ivec3*)scriptField.InstanceFieldValue = { value[0], value[1], value[2] };
								break;
							case ScriptFieldType::Int4:
								*(glm::ivec4*)scriptField.InstanceFieldValue = { value[0], value[1], value[2], value[3] };
								break;

							case ScriptFieldType::Bool:
								*(bool*)scriptField.InstanceFieldValue = value;
								break;
							}
						}
					}
				}
			}
		}

		// Deserialize child entities
		if (jsonObj.contains("Entities"))
		{
			const json& entities = jsonObj["Entities"];
			for (auto it = entities.rbegin(); it != entities.rend(); it++)
				entity.AddChildEntity(DeserializeEntity(*it), false);
		}

		return entity;
	}

}
