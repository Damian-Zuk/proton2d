#pragma once
#include "Proton/Scene/Entity.h"

namespace proton {
	
	using json = nlohmann::ordered_json;

	class Scene;

	class PrefabManager
	{
	public:
		static void Init();

		static void ReloadAll();

		static void SaveEntityAsPrefab(Entity entity);

		static bool LoadPrefab(const std::string& prefabPath);
		static bool DeletePrefab(const std::string& prefabPath);

		static json* GetPrefabJson(UUID prefabUUID);
		static json* GetPrefabJson(const std::string& prefabPath);

		static Entity DeserializePrefab(Entity target, UUID prefabUUID);
		
		static Entity Spawn(Scene* scene, const std::string& prefabPath, UUID entityUUID = UUID());

		static Entity Spawn(Scene* scene, UUID prefabUUID, UUID entityUUID = UUID());

		static bool Exists(const std::string& prefabPath);
		static bool Exists(UUID prefabUUID);

		
	private:
		inline static PrefabManager* s_Instance = nullptr;
		
		friend class Application;
		friend class PrefabPanel;
	};

}
