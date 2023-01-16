#pragma once
#include "proton/Scene/Entity.h"

#include <nlohmann/json.hpp>
#include <stdio.h>

namespace proton {

	using json = nlohmann::ordered_json;
	class Scene;

	class PrefabManager
	{
	public:
		static void Init();

		static void ReloadAllPrefabs();

		static void SaveAsPrefab(Entity entity);
		static bool LoadPrefab(const std::string& prefabName);
		static bool DeletePrefab(const std::string& prefabName);
		static json GetJsonData(const std::string& prefabName);

		static Entity SpawnPrefab(Scene* scene, const std::string& prefabName);

		static bool Exists(const std::string& prefabName);
		
	private:
		static PrefabManager* s_Instance;

		std::unordered_map<std::string, json> m_PrefabsJsonData;
		
		friend class Application;
		friend class EditorOverlay;
	};

}