#pragma once
#include "Proton/Scene/Entity.h"

namespace proton {

	class Scene;

	class PrefabManager
	{
	public:
		static void Init();

		static void ReloadAll();

		static void SaveEntityAsPrefab(Entity entity);

		static bool LoadPrefab(const std::string& prefabPath);
		static bool DeletePrefab(const std::string& prefabPath);

		static Entity Spawn(Scene* scene, const std::string& prefabPath, UUID uuid = 0);
		static Entity Spawn(Scene* scene, UUID prefabUUID, UUID uuid = 0);

		static bool Exists(const std::string& prefabPath);
		static bool Exists(UUID prefabUUID);
		
	private:
		inline static PrefabManager* s_Instance = nullptr;
		
		friend class Application;
		friend class PrefabPanel;
	};

}
