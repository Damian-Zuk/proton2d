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
		enum class FormatType
		{
			Scene = 0,
			Prefab = 1,
		};
		FormatType Format;

	public:
		SceneSerializer() = delete;
		SceneSerializer(Scene* scene, FormatType format = FormatType::Scene);

		virtual ~SceneSerializer() = default;

		std::string SerializeScene();
		bool DeserializeScene(const std::string& jsonData);

		json SerializeEntity(Entity entity);
		Entity DeserializeEntity(const json& data, Entity entity = Entity());

		bool SerializeToFile(const std::string& filepath);
		bool DeserializeFromFile(const std::string& filepath);
		std::string SerializeEntityToString(Entity entity);

		void SetScene(Scene* scene) { m_Scene = scene; }
		
	private:
		void SerializeChildren(Entity entity, json& out);
		void DeserializeChildren(Entity entity, const json& data);

		template<typename TComponent>
		void TrySerialize(std::string_view key, Entity entity, json& out);

		template<typename TComponent>
		void TryDeserialize(std::string_view key, Entity entity, const json& data);

		template<typename TComponent>
		void Serialize(Entity entity, const TComponent& c, json& out);

		template<typename TComponent>
		void Deserialize(Entity entity, TComponent& c, const json& data);

	private:
		Scene* m_Scene;

		// State
		bool m_IsRootEntity = true;
		bool m_IsParentPrefab = false;
	};

}
