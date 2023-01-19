#include "pch.h"
#include "proton/Scene/PrefabManager.h"
#include "proton/Assets/SceneSerializer.h"
#include "proton/Core/Utils.h"

#include <fstream>

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
		for (auto prefabFile : Utils::GetFilesFromDirectory("prefabs", { ".prefab" }, false))
			LoadPrefab(prefabFile);
	}

	void PrefabManager::SaveAsPrefab(Entity entity)
	{
		SceneSerializer serializer(entity.GetScene());
		json jsonData = serializer.SerializeEntity(entity);
		std::string tag = jsonData["Tag"];
		s_Instance->m_PrefabsJsonData[tag] = jsonData;
		std::ofstream file("prefabs/" + tag + ".prefab");
		file << jsonData.dump(4);
		file.close();
	}

	bool PrefabManager::LoadPrefab(const std::string& prefabName)
	{
		std::string rawData = Utils::ReadFile("prefabs/" + prefabName + ".prefab");
		if (rawData.size())
		{
			json jsonData = json::parse(rawData);
			if (jsonData.contains("Tag"))
				s_Instance->m_PrefabsJsonData[jsonData["Tag"]] = jsonData;
			return true;
		}
		return false;
	}

	bool PrefabManager::DeletePrefab(const std::string& prefabName)
	{
		if (Exists(prefabName))
		{
			if (remove(("prefabs/" + prefabName + ".prefab").c_str()) == 0)
			{
				s_Instance->m_PrefabsJsonData.erase(prefabName);
				return true;
			}
		}
		return false;
	}

	json PrefabManager::GetJsonData(const std::string& prefabName)
	{
		assert(Exists(prefabName) && "Prefab not found");
		return s_Instance->m_PrefabsJsonData.at(prefabName);
	}

	bool PrefabManager::Exists(const std::string& prefabName)
	{
		return s_Instance->m_PrefabsJsonData.find(prefabName) != s_Instance->m_PrefabsJsonData.end();
	}

	Entity PrefabManager::SpawnPrefab(Scene* scene, const std::string& prefabName)
	{
		SceneSerializer serializer(scene);
		Entity entity = serializer.DeserializeEntity(GetJsonData(prefabName), false);
		auto camera = scene->GetPrimaryCameraPosition();
		auto& transform = entity.GetComponent<TransformComponent>();
		transform.Position.x = camera.x;
		transform.Position.y = camera.y;
		return entity;
	}

}