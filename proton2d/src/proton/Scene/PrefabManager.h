#pragma once
#include "Proton/Scene/Entity.h"

#include <nlohmann/json.hpp>

namespace proton {

	using json = nlohmann::ordered_json;
	class Scene;

	// TODO: Very basic, needs to be reworked
	class PrefabManager
	{
	public:
		static void Init();

		static void ReloadAll();

		static void CreatePrefabFromEntity(Entity entity);

		static bool LoadPrefab(const std::string& prefabPath);
		static bool DeletePrefab(const std::string& prefabPath);

		static Entity Spawn(Scene* scene, const std::string& prefabPath);

		static bool Exists(const std::string& prefabPath);
		
	private:
		static PrefabManager* s_Instance;

		std::map<std::string, json> m_PrefabsJsonData;
		
		friend class Application;
		friend class PrefabPanel;
	};

}
