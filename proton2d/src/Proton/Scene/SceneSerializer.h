#pragma once
#include "Proton/Core/UUID.h"
#include "Proton/Scene/Entity.h"

namespace proton {

	using json = nlohmann::ordered_json;

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
		void ResetState();

		void SerializeChildEntities(Entity entity, json& out);
		void DeserializeChildEntities(Entity entity, const json& data);

		inline bool AreComponentsSerialized(bool isPrefabEntity) const;
		inline bool IsPositionSerialized() const;
		inline bool IsRotationAndScaleSerialized(bool isPrefabEntity) const;

		template<typename TComponent>
		void TrySerialize(std::string_view key, Entity entity, json& j);

		template<typename TComponent>
		void TryDeserialize(std::string_view key, Entity entity, const json& j);

		template<typename TComponent>
		void Serialize(Entity entity, const TComponent& c, json& j);

		template<typename TComponent>
		void Deserialize(Entity entity, TComponent& c, const json& j);

	private:
		Scene* m_Scene;

		// State
		int32_t m_HierarchyLevel = 0;
		int32_t m_ParentPrefabLevel = -1;
		int32_t m_NestedPrefabsCount = 0;
	};

}
