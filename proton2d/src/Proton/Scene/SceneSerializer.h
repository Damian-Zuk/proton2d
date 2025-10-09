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
		Entity DeserializeEntity(const json& j, Entity entity = Entity());

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
		void UpdateHierarchyTraversalState();

		bool AreDataComponentsSerialized() const;
		bool IsPositionSerialized() const;

		void SerializeChildEntities(Entity entity, json& j);
		void DeserializeChildEntities(Entity entity, const json& j);

		template<typename TComponent>
		void Serialize(Entity entity, const TComponent& c, json& j);
		template<typename TComponent>
		void Deserialize(Entity entity, TComponent& c, const json& j);

		template<typename TComponent>
		static constexpr std::string_view GetComponentName();

		template<typename TComponent>
		void TrySerialize(Entity entity, json& j, bool useRootObject = false);
		template<typename TComponent>
		void TryDeserialize(Entity entity, const json& j, bool useRootObject = false);

		template<typename... TComponent>
		void TrySerializeComponents(Entity entity, json& j, bool useRootObject = false);
		template<typename... TComponent>
		void TryDeserializeComponents(Entity entity, const json& j, bool useRootObject = false);

		template<typename... TComponent>
		void TrySerializeComponents(ComponentGroup<TComponent...>, Entity entity, json& j, bool useRootObject = false);
		template<typename... TComponent>
		void TryDeserializeComponents(ComponentGroup<TComponent...>, Entity entity, const json& j, bool useRootObject = false);

	private:
		Scene* m_Scene;

		struct HierarchyTraversalState
		{
			int32_t HierarchyLevel = 0;
			int32_t ParentPrefabLevel = -1;
			bool IsCurrentPrefab = false;
			bool IsNestedPrefab = false;

		} m_State;
	};

}
