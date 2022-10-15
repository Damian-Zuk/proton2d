#pragma once

#include "proton/Core/Core.h"
#include "proton/Entity/Components.h"
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

		bool IsValid()
		{
			if (!m_Scene->m_Registry.valid(m_Handle))
			{
				m_Handle = entt::null;
				return false;
			}
			return true;
		}

		void Destroy(bool skipParentRelationsCheck = false)
		{
			auto& r = GetComponent<RelationshipComponent>();

			if (!skipParentRelationsCheck && r.Parent != entt::null)
			{
				// Update relationship linked list
				Entity parent{ m_Scene, r.Parent };
				Entity prev{ m_Scene, r.Prev };
				Entity next{ m_Scene, r.Next };

				auto& parentReletions = parent.GetComponent<RelationshipComponent>();
				parentReletions.ChildrenCount--;

				if (prev)
					prev.GetComponent<RelationshipComponent>().Next = next.m_Handle;
				else
					parentReletions.First = r.Next;

				if (next)
					next.GetComponent<RelationshipComponent>().Prev = prev.m_Handle;
				else
					parentReletions.Last = r.Prev;
			}
			
			if (r.First != entt::null)
			{
				// Destroy child entities
				auto current = r.First;
				for (uint32_t i = 0; i < r.ChildrenCount; i++)
				{
					Entity e = { m_Scene, current };
					auto next = e.GetComponent<RelationshipComponent>().Next;
					e.Destroy(true);
					current = next;
				}
			}

			m_Scene->m_Registry.destroy(m_Handle);
			m_Handle = entt::null;
		}

		void AddChildEntity(Entity& child)
		{
			auto& parentComponent = GetComponent<RelationshipComponent>();
			auto& childComponent = child.GetComponent<RelationshipComponent>();

			childComponent.Parent = m_Handle;

			if (parentComponent.ChildrenCount)
			{
				childComponent.Prev = parentComponent.Last;
				Entity lastEntity = { m_Scene, parentComponent.Last };
				lastEntity.GetComponent<RelationshipComponent>().Next = child.m_Handle;
			}
			else
				parentComponent.First = child.m_Handle;

			parentComponent.Last = child.m_Handle;
			parentComponent.ChildrenCount++;
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