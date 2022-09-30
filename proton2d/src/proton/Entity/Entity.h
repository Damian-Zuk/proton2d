#pragma once

#include "proton/Core/Core.h"
#include "proton/Entity/Scene.h"

namespace proton {

	class Entity
	{
	public:
		Entity() = default;

		Entity(Scene* scene, entt::entity handle)
			: m_Scene(scene), m_Handle(handle) {}

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
			//static_assert(!std::is_base_of<EntityScript, T>, "Use AddScriptComponent function to add script.");
			assert(!HasComponent<T>() && "Entity already have component!");
			return m_Scene->m_Registry.emplace<T>(m_Handle, std::forward(args)...);
		}

		template <typename T>
		void AddScriptComponent(const std::string& scriptName) const
		{
			if (!HasComponent<ScriptComponent>())
				AddComponent<ScriptComponent>();
			
			auto& scriptComponent = GetComponent<ScriptComponent>();
			scriptComponent.BindScript<T>(scriptName);
		}

		template <typename T>
		void RemoveComponent() const
		{
			assert(HasComponent<T>() && "Entity doesn't have component!");
			m_Scene->m_Registry.remove<T>(m_Handle);
		}

		template <typename T>
		bool HasComponent() const
		{
			return m_Scene->m_Registry.any_of<T>(m_Handle);
		}

		template <typename T>
		bool HasScript() const
		{
			if (!HasComponent<ScriptComponent>())
				return false

			auto& scriptComponent = m_Scene->m_Registry.get<ScriptComponent>(m_Handle);
			//return scriptComponent.ScriptInstances.find();
		}

		bool IsValid()
		{
			if (!m_Scene->m_Registry.valid(m_Handle))
			{
				m_Handle = entt::null;
				return false;
			}
			return true;
		}

		void Destroy()
		{
			m_Scene->m_Registry.destroy(m_Handle);
			m_Handle = entt::null;
		}

		operator uint32_t() const { return (uint32_t)m_Handle; }
		operator bool() const { return m_Handle != entt::null; }
		bool operator==(const Entity& other) const { return other.m_Handle == m_Handle; }
		bool operator!=(const Entity& other) const { return other.m_Handle != m_Handle; }

	private:
		Scene* m_Scene = nullptr;
		entt::entity m_Handle = entt::null;

		friend class Scene;
	};

}