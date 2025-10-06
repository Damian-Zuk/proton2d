#pragma once
#include "Proton/Core/UUID.h"
#include "Proton/Scene/Entity.h"
#include <nlohmann/json.hpp>

namespace proton {

	using json = nlohmann::ordered_json;

	class Scene;
	class Entity;

	class SceneSerializer
	{
	public:
		enum class SourceType
		{
			SceneFile = 0,
			PrefabFile,
		};
		SourceType Source = SourceType::SceneFile;

	public:
		SceneSerializer(Scene* scene);
		SceneSerializer() = delete;
		~SceneSerializer() = default;

		void SetScene(Scene* scene);

		std::string Serialize();
		bool SerializeToFile(const std::string& filepath);

		bool Deserialize(const std::string& jsonData);
		bool DeserializeFromFile(const std::string& filepath);

		json SerializeEntity(Entity entity);
		std::string SerializeEntityToString(Entity entity);

		Entity DeserializeEntity(const json& data, Entity entity = Entity());
		
	private:
		void SerializeChildren(Entity entity, json& out);
		void DeserializeChildren(Entity entity, const json& data);

	private:
		Scene* m_Scene;
		bool m_IsRootEntity = true;
		bool m_IsParentPrefab = false;
	};

}
