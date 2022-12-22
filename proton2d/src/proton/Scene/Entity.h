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

		template <typename T>
		T& GetComponent() const
		{
			assert(HasComponent<T>() && "Entity doesn't have component!");
			return m_Scene->m_Registry.get<T>(m_Handle);
		}

		template <typename T, typename... Types>
		T& AddComponent(Types&& ...args) const
		{
			assert(!HasComponent<T>() && "Entity already have component!");
			return m_Scene->m_Registry.emplace<T>(m_Handle, std::forward<Types>(args)...);
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
		FlipbookAnimationComponent& AddComponent() const
		{
			assert(!HasComponent<FlipbookAnimationComponent>() && "Entity already have component!");
			assert(HasComponent<SpriteComponent>() && "Entity must have sprite component");
			auto& sprite = GetComponent<SpriteComponent>();
			assert(sprite.Sprite && sprite.Sprite->m_SpriteSheet && "Entity must have spritesheet texture");

			auto& fb = m_Scene->m_Registry.emplace<FlipbookAnimationComponent>(m_Handle);
			fb.Flipbook = CreateShared<Flipbook>(sprite.Sprite);
			return fb;
		}

		template <typename T>
		EntityScript* AddScript(const std::string& scriptName) const
		{
			if (!HasComponent<ScriptComponent>())
				AddComponent<ScriptComponent>();
			
			return GetComponent<ScriptComponent>().Attach<T>(scriptName);
		}

		template <typename T>
		void RemoveComponent() const
		{
			assert(HasComponent<T>() && "Entity doesn't have component!");
			
			if (std::is_base_of<ScriptComponent, T>::value)
			{
				for (auto& [scriptName, scriptData] : GetComponent<ScriptComponent>().Scripts)
					scriptData.DestroyInstanceFunction();
			}
			m_Scene->m_Registry.remove<T>(m_Handle);
		}

		template <typename T>
		bool HasComponent() const
		{
			return m_Scene->m_Registry.any_of<T>(m_Handle);
		}

		template <typename... TComponents>
		bool HasComponents() const
		{
			return m_Scene->m_Registry.all_of<TComponents...>(m_Handle);
		}

		UUID GetUUID() const;
		bool IsValid();
		void Destroy();
		void AddChildEntity(Entity child);
		void DestroyChildEntities();

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
		operator bool() const { return m_Handle != entt::null; }
		bool operator==(const Entity& other) const { return other.m_Handle == m_Handle; }
		bool operator!=(const Entity& other) const { return !(other == *this); }

	private:
		Scene* m_Scene = nullptr;
		entt::entity m_Handle = entt::null;

		friend class Scene;
		friend class SceneSerializer;
		friend class EntityScript;
		friend class Inspector;
	};

}