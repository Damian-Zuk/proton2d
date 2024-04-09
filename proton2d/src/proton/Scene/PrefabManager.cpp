#include "ptpch.h"
#include "Proton/Scene/PrefabManager.h"
#include "Proton/Assets/SceneSerializer.h"
#include "Proton/Utils/Utils.h"
#include "Proton/Core/GameInstance.h"
#include "Proton/Network/Common/NetworkManager.h"
#include "Proton/Network/Server/Server.h"

#include <fstream>
#ifdef PT_EDITOR
#include "Proton/Editor/EditorLayer.h"
#endif

namespace proton {

	PrefabManager* PrefabManager::s_Instance = nullptr;

	void PrefabManager::Init()
	{
		if (!s_Instance)
		{
			s_Instance = new PrefabManager();
			s_Instance->ReloadAllPrefabs();
		}
	}

	void PrefabManager::ReloadAllPrefabs()
	{
		s_Instance->m_PrefabsJsonData.clear();
		for (const auto& prefabFile : Utils::ScanDirectoryRecursive("content/prefabs", { ".prefab.json" }))
			LoadPrefab(prefabFile);
	}

	void PrefabManager::CreatePrefabFromEntity(Entity entity)
	{
		SceneSerializer serializer(entity.GetScene());
		json jsonData = serializer.SerializeEntity(entity);
		std::string tag = jsonData["Tag"];
		s_Instance->m_PrefabsJsonData[tag] = jsonData;
		std::ofstream file("content/prefabs/" + tag + ".prefab.json");
		file << jsonData.dump(4);
		file.close();
	}

	bool PrefabManager::LoadPrefab(const std::string& prefabPath)
	{
		std::string rawData = Utils::ReadFile("content/prefabs/" + prefabPath);
		if (rawData.size())
		{
			json jsonData = json::parse(rawData);
			s_Instance->m_PrefabsJsonData[prefabPath] = jsonData;
			return true;
		}
		return false;
	}

	bool PrefabManager::DeletePrefab(const std::string& prefabPath)
	{
		if (Exists(prefabPath))
		{
			if (remove(("content/prefabs/" + prefabPath).c_str()) == 0)
			{
				s_Instance->m_PrefabsJsonData.erase(prefabPath);
				return true;
			}
		}
		return false;
	}

	bool PrefabManager::Exists(const std::string& prefabPath)
	{
		return s_Instance->m_PrefabsJsonData.find(prefabPath) != s_Instance->m_PrefabsJsonData.end();
	}

	Entity PrefabManager::SpawnPrefab(Scene* scene, const std::string& prefabPath)
	{
		if (!Exists(prefabPath))
		{
			if (!LoadPrefab(prefabPath))
			{
				PT_CORE_ERROR("Prefab '{}' not found", prefabPath);
				return Entity();
			}
		}

		SceneSerializer serializer(scene);
		const json& prefabData = s_Instance->m_PrefabsJsonData.at(prefabPath);
		Entity entity = serializer.DeserializeEntity(prefabData, false);
		
		auto camera = scene->GetPrimaryCameraPosition();
		auto& transform = entity.GetComponent<TransformComponent>();
		transform.WorldPosition.x = camera.x;
		transform.WorldPosition.y = camera.y;

		// TODO: move this 
		//if (scene->IsSimulated())
		//{
		//	NetworkManager* manager = scene->GetOwningGameInstance()->GetNetworkManager();
		//	if (manager->IsNetModeServer())
		//	{
		//		manager->GetServer()->OnEntityCreated(entity);
		//	}
		//}
		
		return entity;
	}

}
