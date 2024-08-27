#pragma once
#include "Proton/Core/UUID.h"
#include <nlohmann/json.hpp>

namespace proton {

	using json = nlohmann::ordered_json;

	class Scene;
	class Entity;

	class SceneSerializer
	{
	public:
		bool IsPrefabSerializer = false;

	public:
		SceneSerializer(Scene* scene);
		SceneSerializer() = delete;
		~SceneSerializer() = default;

		bool SerializeToFile(const std::string& filepath);
		std::string Serialize();

		json SerializeEntity(Entity entity);
		std::string SerializeEntityToString(Entity entity);

		bool Deserialize(const std::string& jsonData);
		bool DeserializeFromFile(const std::string& filepath);

		Entity DeserializeEntity(const json& jsonObj, UUID uuid = 0);
		
	private:
		Scene* m_Scene;
	};

}
