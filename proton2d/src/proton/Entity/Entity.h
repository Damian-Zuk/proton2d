#pragma once

#include "proton/Core/Core.h"
#include "proton/Entity/Components.h"
#include "proton/Entity/Scene.h"

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
			return m_Scene->m_Registry.emplace<T>(m_Handle, std::forward(args)...);
		}

		template <typename T>
		void AddScript(const std::string& scriptName) const
		{
			if (!HasComponent<ScriptComponent>())
				AddComponent<ScriptComponent>();
			
			GetComponent<ScriptComponent>().Bind<T>(scriptName);
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

		bool IsValid();
		void Destroy();
		void AddChildEntity(Entity& child);
		void DestroyChildEntities();

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