#pragma once

#include "proton/Core/Core.h"
#include "proton/Scene/Components.h"
#include "proton/Scene/Scene.h"

namespace proton {

	class Entity
	{
	public:
		Entity() = default;
		Entity(Scene* scene, entt::entity handle);

		virtual ~Entity() = default;

		// Returns specified component of entity
		template <typename T>
		T& GetComponent() const
		{
			assert(HasComponent<T>() && "Entity doesn't have component!");
			return m_Scene->m_Registry.get<T>(m_Handle);
		}

		// Adds component to entity
		template <typename T, typename... Types>
		T& AddComponent(Types&& ...args) const
		{
			assert(!HasComponent<T>() && "Entity already have component!");
			return m_Scene->m_Registry.emplace<T>(m_Handle, std::forward<Types>(args)...);
		}

		template<>
		NineSliceSpriteComponent& AddComponent() const
		{
			auto& sprite = m_Scene->m_Registry.emplace<NineSliceSpriteComponent>(m_Handle);
			sprite.NineSliceSprite.SetTransform(&GetComponent<TransformComponent>());
			return sprite;
		}

		template<>
		CameraComponent& AddComponent() const
		{
			assert(!HasComponent<CameraComponent>() && "Entity already have component!");
			auto& camera = m_Scene->m_Registry.emplace<CameraComponent>(m_Handle);
			camera.Camera = CreateShared<Camera>();
			return camera;
		}

		template<>
		SpriteAnimationComponent& AddComponent() const
		{
			assert(!HasComponent<SpriteAnimationComponent>() && "Entity already have component!");
			assert(HasComponent<SpriteComponent>() && "Entity must have sprite component");
			auto& sprite = GetComponent<SpriteComponent>().Sprite;
			assert(sprite.m_Spritesheet && "Entity must have spritesheet texture");

			auto& fb = m_Scene->m_Registry.emplace<SpriteAnimationComponent>(m_Handle);
			fb.SpriteAnimation = CreateShared<SpriteAnimation>(&sprite);
			return fb;
		}

		// Adds script to entity and returns it's instance
		template <typename TScriptClass>
		EntityScript* AddScript() const
		{
			if (!HasComponent<ScriptComponent>())
				AddComponent<ScriptComponent>();

			auto& component = GetComponent<ScriptComponent>();
			std::string className = TScriptClass::__ScriptClassName;
			assert(component.Scripts.find(className) == component.Scripts.end()
				&& "Entity has already a script added");

			EntityScript*& scriptInstance = component.Scripts[className];
			scriptInstance = new TScriptClass();
			scriptInstance->OnRegisterFields();
			return scriptInstance;
		}

		// Removes script from entity
		void RemoveScript(const std::string& scriptClassName);

		// Removes component from entity
		template <typename T>
		void RemoveComponent()
		{
			assert(HasComponent<T>() && "Entity doesn't have a component!");
			
			if (std::is_base_of<ScriptComponent, T>::value)
				DeleteAllScriptInstances();

			if (std::is_base_of<CameraComponent, T>::value
				&& m_Scene->m_PrimaryCameraEntity == *this)
			{
				m_Scene->m_PrimaryCameraEntity = entt::null;
				m_Scene->m_PrimaryCamera = nullptr;
			}

			m_Scene->m_Registry.remove<T>(m_Handle);
		}

		// Checks if entity has given component
		template <typename T>
		bool HasComponent() const
		{
			return m_Scene->m_Registry.any_of<T>(m_Handle);
		}

		// Checks if entity has specified set of components
		template <typename... TComponents>
		bool HasComponents() const
		{
			return m_Scene->m_Registry.all_of<TComponents...>(m_Handle);
		}

		// Returns TransformComponent
		TransformComponent& GetTransform();

		// Returns pointer to scene
		Scene* GetScene() { return m_Scene; }
		// Returns entity unique identifier
		UUID GetUUID() const;
		// Returns entity tag stored in TagComponent
		const std::string& GetTag() const;

		// Checks if entity is valid
		bool IsValid();
		// Destroys entity and it's child entities
		void Destroy();

		// Adds child entity given as parameter
		void AddChildEntity(Entity child);
		// Destroys all child entities
		void DestroyChildEntities();
		// Detaches entity from parent and moves to scene root
		void PopHierarchy();
		// Checks if entity is parent of given entity
		bool IsParentOf(Entity entity);

		// Requires RigidbodyComponent
		b2Body* GetBox2DRigidbody();

		// Requires RigidbodyComponent
		void SetVelocity(float x_mps, float y_mps);
		// Requires RigidbodyComponent
		void SetVelocityX(float mps);
		// Requires RigidbodyComponent
		void SetVelocityY(float mps);

		// Requires RigidbodyComponent
		glm::vec2 GetVelocity();

		// Requires RigidbodyComponent
		void ApplyImpulse(const glm::vec2& impulse);

		operator uint32_t() const { return (uint32_t)m_Handle; }
		operator entt::entity() const { return m_Handle; }
		operator bool() const { return m_Handle != entt::null; }
		bool operator==(const Entity& other) const { return other.m_Handle == m_Handle; }
		bool operator!=(const Entity& other) const { return !(other == *this); }

	private:
		void DeleteAllScriptInstances();

		Scene* m_Scene = nullptr;
		entt::entity m_Handle = entt::null;

		friend class Scene;
		friend class SceneSerializer;
		friend class EntityScript;
		friend class Inspector;
		friend class EditorOverlay;
	};

}