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
		static PrefabManager& Get();

		void ReloadAllPrefabs();

		bool LoadPrefab(const std::string& prefabName);
		void SavePrefab(Entity entity);
		bool DeletePrefab(const std::string& prefabName);
		
		Entity SpawnPrefab(Scene* scene, const std::string& prefabName) const;

		bool Exists(const std::string& prefabName) const;
		
	private:
		std::unordered_map<std::string, json> m_PrefabsJsonData;
		
		friend class Application;
		friend class EditorOverlay;
	};

}