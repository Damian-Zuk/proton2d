#include "ptpch.h"
#include "Proton/Scene/PrefabManager.h"
#include "Proton/Scene/SceneSerializer.h"
#include "Proton/Utils/Utils.h"
#include "Proton/Core/GameInstance.h"
#include "Proton/Network/NetworkManager.h"
#include "Proton/Network/Server.h"

#ifdef PT_EDITOR
#include "Proton/Editor/EditorLayer.h"
#endif

#include <fstream>
#include <filesystem>

namespace proton {

	struct
	{
		std::vector<Shared<json>> PersistentStorage;

		std::unordered_map<std::string, json*> PathToJsonMap;
		std::unordered_map<UUID, json*> UUIDToJsonMap;

	} static s_Data;

	static void AddPrefabToStorage(const json& entityJson, const std::string& path)
	{
		s_Data.PersistentStorage.push_back(MakeShared<json>(entityJson));
		s_Data.PathToJsonMap[path] = s_Data.PersistentStorage.back().get();
		if (entityJson.contains("PrefabUUID"))
		{
			uint64_t uuid = (uint64_t)entityJson.at("PrefabUUID");
			s_Data.UUIDToJsonMap[uuid] = s_Data.PersistentStorage.back().get();
		}
	}

	static void RemovePrefabFromStorage(const std::string& path)
	{
		auto entityJsonIt = s_Data.PathToJsonMap.find(path);
		
		if (entityJsonIt != s_Data.PathToJsonMap.end())
		{
			json* entityJson = entityJsonIt->second;
			UUID uuid = entityJson->at("PrefabUUID");

			auto& s = s_Data.PersistentStorage;
			s.erase(s.begin(), std::find_if(s.begin(), s.end(), [&](const Shared<json>& data) {
					return (uint64_t)data->at("PrefabUUID") == uuid;
				})
			);
			s_Data.PathToJsonMap.erase(path);
			s_Data.UUIDToJsonMap.erase(uuid);
		}

	}

	void PrefabManager::Init()
	{
		if (!s_Instance)
		{
			s_Instance = new PrefabManager();
			s_Instance->ReloadAll();
		}
	}

	void PrefabManager::ReloadAll()
	{
		s_Data.PersistentStorage.clear();
		s_Data.PathToJsonMap.clear();
		s_Data.UUIDToJsonMap.clear();

		for (const auto& entry : std::filesystem::recursive_directory_iterator("content/prefabs"))
		{
			if (entry.is_directory())
				continue;

			auto& path = std::filesystem::relative(entry.path(), "content/prefabs");
			std::string filepath = path.replace_extension().replace_extension().string();
			std::replace(filepath.begin(), filepath.end(), '\\', '/');

			LoadPrefab(filepath);
		}
	}

	void PrefabManager::SaveEntityAsPrefab(Entity entity)
	{
		if (!entity.HasComponent<PrefabComponent>())
		{
			auto& pc = entity.AddComponent<PrefabComponent>();
			pc.PrefabUUID = entity.GetUUID();
		}

		SceneSerializer serializer(entity.GetScene());
		serializer.Format = SceneSerializer::FormatType::Prefab;
		json jsonData = serializer.SerializeEntity(entity);
		std::string tag = entity.GetTag();
		std::ofstream file("content/prefabs/" + tag + ".prefab.json");
		file << jsonData.dump(4);
		file.close();
		
		AddPrefabToStorage(jsonData, tag);

		Scene* scene = entity.GetScene();
		auto view = scene->GetAllEntitiesWith<PrefabComponent>();
		for (entt::entity e : view)
		{
			const auto& prefab = view.get<PrefabComponent>(e);
			if (prefab.PrefabUUID == entity.GetPrefabUUID())
			{
				DeserializePrefab(Entity(e, scene), prefab.PrefabUUID);
			}
		}

	}

	bool PrefabManager::LoadPrefab(const std::string& prefabPath)
	{
		std::string rawData = Utils::ReadFile("content/prefabs/" + prefabPath + ".prefab.json");
		if (rawData.size())
		{
			AddPrefabToStorage(json::parse(rawData), prefabPath);
			return true;
		}
		return false;
	}

	bool PrefabManager::DeletePrefab(const std::string& prefabPath)
	{
		if (Exists(prefabPath))
		{
			if (remove(("content/prefabs/" + prefabPath + ".prefab.json").c_str()) == 0)
			{
				RemovePrefabFromStorage(prefabPath);
				return true;
			}
		}
		return false;
	}

	bool PrefabManager::Exists(const std::string& prefabPath)
	{
		return s_Data.PathToJsonMap.find(prefabPath) != s_Data.PathToJsonMap.end();
	}

	bool PrefabManager::Exists(UUID prefabUUID)
	{
		return s_Data.UUIDToJsonMap.find(prefabUUID) != s_Data.UUIDToJsonMap.end();
	}

	json* PrefabManager::GetPrefabJson(UUID prefabUUID)
	{
		if (!Exists(prefabUUID))
		{
			PT_CORE_ERROR("Prefab with uuid='{}' not found", prefabUUID);
			return nullptr;
		}
		return s_Data.UUIDToJsonMap.at(prefabUUID);
	}

	json* PrefabManager::GetPrefabJson(const std::string& prefabPath)
	{
		if (!Exists(prefabPath))
		{
			PT_CORE_ERROR("Prefab with path='{}' not found", prefabPath);
			return nullptr;
		}
		return s_Data.PathToJsonMap.at(prefabPath);
	}

	Entity PrefabManager::DeserializePrefab(Entity target, UUID prefabUUID)
	{
		PT_CORE_VERIFY(target.IsValid());
		SceneSerializer serializer(target.GetScene());
		serializer.Format = SceneSerializer::FormatType::Prefab;
		json* prefabJson = GetPrefabJson(prefabUUID);
		if (prefabJson)
			return serializer.DeserializeEntity(*prefabJson, target);
		return target;
	}

	Entity PrefabManager::Spawn(Scene* scene, UUID prefabUUID, UUID entityUUID)
	{
		SceneSerializer serializer(scene);
		serializer.Format = SceneSerializer::FormatType::Prefab;
		json* prefabJson = GetPrefabJson(prefabUUID);

		if (prefabJson) 
		{
			Entity entity = entityUUID ? scene->CreateEntityWithUUID(entityUUID) : Entity();
			return serializer.DeserializeEntity(*prefabJson, entity);
		}

		return Entity();
	}

	Entity PrefabManager::Spawn(Scene* scene, const std::string& prefabPath, UUID entityUUID)
	{
		SceneSerializer serializer(scene);
		serializer.Format = SceneSerializer::FormatType::Prefab;
		json* prefabJson = GetPrefabJson(prefabPath);
		
		if (prefabJson)
			return serializer.DeserializeEntity(*prefabJson);
		
		return Entity();
	}
}
