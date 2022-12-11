#include "pch.h"
#include "proton/Entity/Entity.h"

namespace proton 
{
	Entity::Entity(Scene* scene, entt::entity handle)
		: m_Scene(scene), m_Handle(handle) 
	{
	}

	bool Entity::IsValid()
	{
		if (!m_Scene->m_Registry.valid(m_Handle))
		{
			m_Handle = entt::null;
			return false;
		}
		return true;
	}

	void Entity::DestroyChildEntities()
	{
		auto& rc = GetComponent<RelationshipComponent>();
		entt::entity current = rc.First;

		for (uint32_t i = 0; i < rc.ChildrenCount; i++)
		{
			Entity childEntity = { m_Scene, current };
			auto& childRC = childEntity.GetComponent<RelationshipComponent>();
			entt::entity next = childRC.Next;

			childEntity.DestroyChildEntities();
			m_Scene->m_Registry.destroy(current);

			current = next;
		}
	}

	void Entity::Destroy()
	{
		auto& rc = GetComponent<RelationshipComponent>();

		DestroyChildEntities();

		if (rc.Parent != entt::null)
		{
			Entity parent { m_Scene, rc.Parent };
			Entity prev   { m_Scene, rc.Prev   };
			Entity next   { m_Scene, rc.Next   };

			auto& parentReletions = parent.GetComponent<RelationshipComponent>();
			parentReletions.ChildrenCount--;

			if (prev)
				prev.GetComponent<RelationshipComponent>().Next = next.m_Handle;
			else
				parentReletions.First = rc.Next;

			if (next)
				next.GetComponent<RelationshipComponent>().Prev = prev.m_Handle;
		}

		m_Scene->m_Registry.destroy(m_Handle);
		m_Handle = entt::null;
	}

	void Entity::AddChildEntity(Entity child)
	{
		auto& parentComponent = GetComponent<RelationshipComponent>();
		auto& childComponent = child.GetComponent<RelationshipComponent>();

		childComponent.Parent = m_Handle;

		if (parentComponent.ChildrenCount)
		{
			Entity firstEntity = { m_Scene, parentComponent.First };
			firstEntity.GetComponent<RelationshipComponent>().Prev = child.m_Handle;
			childComponent.Next = parentComponent.First;
		}

		parentComponent.First = child.m_Handle;
		parentComponent.ChildrenCount++;
	}
}