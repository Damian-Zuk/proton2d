#pragma once
#include "proton/Scene/Entity.h"

#include <nlohmann/json.hpp>
#include <stdio.h>

namespace proton {

	using json = nlohmann::ordered_json;
	class Scene;

	/*
	* Class PrefabManager is used to create, load and spawn prefabs
	* PrefabManager class methods use prefab filepaths - "prefabPath"
	* (realative to "prefabs" directory) without ".prefab" extension
	* as prefab identifiers (keys in map storage).
	* Prefabs are stored as JSON object data, which is used for
	* deserialization in SpawnPrefab method.
	*/
	class PrefabManager
	{
	public:
		static void Init();

		static void ReloadAllPrefabs();

		static void CreatePrefabFromEntity(Entity entity);
		static bool LoadPrefab(const std::string& prefabPath);
		static bool DeletePrefab(const std::string& prefabPath);

		static Entity SpawnPrefab(Scene* scene, const std::string& prefabPath);

		static bool Exists(const std::string& prefabPath);
		
	private:
		static PrefabManager* s_Instance;

		std::map<std::string, json> m_PrefabsJsonData;
		
		friend class Application;
		friend class PrefabPanel;
	};

}
