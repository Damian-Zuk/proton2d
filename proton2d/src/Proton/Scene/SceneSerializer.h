#pragma once
#include "Proton/Core/UUID.h"
#include "Proton/Scene/Entity.h"

namespace proton {

	class SceneSerializer
	{
	public:
		enum class FormatType
		{
			Scene = 0,
			Prefab = 1,
		};
		FormatType Format = FormatType::Scene;

	public:
		SceneSerializer(Scene* scene);

		SceneSerializer() = delete;
		virtual ~SceneSerializer() = default;

		json SerializeScene();
		bool DeserializeScene(const json& j);

		json SerializeEntity(Entity entity);
		Entity DeserializeEntity(const json& data, Entity entity = Entity());

		std::string SerializeSceneToString();
		bool SerializeSceneToFile(std::string_view filepath);
		bool DeserializeSceneFromString(std::string_view jsonData);
		bool DeserializeSceneFromFile(std::string_view filepath);

		std::string SerializeEntityToString(Entity entity);
		bool SerializeEntityToFile(Entity entity, std::string_view filepath);
		Entity DeserializeEntityFromString(std::string_view jsonData, Entity entity = Entity());
		Entity DeserializeEntityFromFile(std::string_view filepath, Entity entity = Entity());

		void SetScene(Scene* scene) { m_Scene = scene; }
		
	private:
		void ResetHierarchyState();
		void UpdateHierarchyState(Entity entity);

		inline bool AreComponentsSerialized(bool isPrefabEntity) const;
		inline bool IsPositionSerialized() const;
		inline bool IsRotationAndScaleSerialized(bool isPrefabEntity) const;

		void SerializeChildEntities(Entity entity, json& j);
		void DeserializeChildEntities(Entity entity, const json& j);

		template<typename... TComponent>
		void DeserializeComponents(Entity entity, const json& j);
		template<typename... TComponent>
		void DeserializeComponents(ComponentGroup<TComponent...>, Entity entity, const json& j);

		template<typename... TComponent>
		void SerializeComponents(Entity entity, json& j);
		template<typename... TComponent>
		void SerializeComponents(ComponentGroup<TComponent...>, Entity entity, json& j);

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

		// State for hierarchy traversal tracking
		enum { None = -1 };
		int32_t m_ParentPrefabLevel = None;
		int32_t m_HierarchyLevel = 0;
		int32_t m_NestedPrefabs = 0;
	};

}
