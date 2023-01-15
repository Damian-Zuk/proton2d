#include "pch.h"
#include "proton/Scene/PrefabManager.h"
#include "proton/Assets/SceneSerializer.h"
#include "proton/Core/Utils.h"

#include <fstream>

namespace proton {

	PrefabManager& PrefabManager::Get()
	{
		static PrefabManager manager;
		return manager;
	}

	void PrefabManager::ReloadAllPrefabs()
	{
		m_PrefabsJsonData.clear();
		for (auto prefabFile : GetFilesFromDirectory("prefabs", "prefab"))
			LoadPrefab(prefabFile);
	}

	bool PrefabManager::LoadPrefab(const std::string& prefabName)
	{
		std::string rawData = ReadFileBinary("prefabs/" + prefabName + ".prefab");
		if (rawData.size())
		{
			json jsonData = json::parse(rawData);
			if (jsonData.contains("Tag"))
				m_PrefabsJsonData[jsonData["Tag"]] = jsonData;
			return true;
		}
		return false;
	}

	void PrefabManager::SavePrefab(Entity entity)
	{
		SceneSerializer serializer(entity.GetScene());
		json jsonData = serializer.SerializeEntity(entity, false);
		std::string tag = jsonData["Tag"];
		m_PrefabsJsonData[tag] = jsonData;
		std::ofstream file("prefabs/" + tag + ".prefab");
		file << jsonData.dump(4);
		file.close();
	}

	bool PrefabManager::DeletePrefab(const std::string& prefabName)
	{
		if (Exists(prefabName))
		{
			if (remove(("prefabs/" + prefabName + ".prefab").c_str()) == 0)
			{
				m_PrefabsJsonData.erase(prefabName);
				return true;
			}
		}
		return false;
	}

	bool PrefabManager::Exists(const std::string& prefabName) const
	{
		return m_PrefabsJsonData.find(prefabName) != m_PrefabsJsonData.end();
	}

	Entity PrefabManager::SpawnPrefab(Scene* scene, const std::string& prefabName) const
	{
		assert(Exists(prefabName) && "Prefab not found");
		SceneSerializer serializer(scene);
		return serializer.DeserializeEntity(m_PrefabsJsonData.at(prefabName), false);
	}

}