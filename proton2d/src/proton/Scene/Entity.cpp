#include "pch.h"
#include "proton/Scene/Entity.h"
#include "proton/Scene/EntityScript.h"

#include <box2d/b2_body.h>

namespace proton 
{
	Entity::Entity(Scene* scene, entt::entity handle)
		: m_Scene(scene), m_Handle(handle) 
	{
	}

	Entity Entity::CopyEntity(Scene* dstScene)
	{
		return m_Scene->CopyEntity(*this, dstScene);
	}

	UUID Entity::GetUUID() const
	{
		return GetComponent<IDComponent>().ID;
	}

	const std::string& Entity::GetTag() const
	{
		return GetComponent<TagComponent>().Tag;
	}

	bool Entity::IsValid()
	{
		if (m_Handle == entt::null)
			return false;

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
			m_Scene->DestroyEntity(childEntity);

			current = next;
		}
	}

	b2Body* Entity::GetBox2DRigidbody()
	{
		if (!HasComponent<RigidbodyComponent>())
			return nullptr;

		auto& id = GetComponent<IDComponent>();
		return m_Scene->GetBox2DRuntimeBody(id.ID);
	}

	void Entity::SetVelocity(float x_mps, float y_mps)
	{
		b2Body* body = GetBox2DRigidbody();
		body->SetLinearVelocity({ x_mps, y_mps });
	}

	void Entity::SetVelocityX(float mps)
	{
		b2Body* body = GetBox2DRigidbody();
		body->SetLinearVelocity({ mps, body->GetLinearVelocity().y });
	}

	void Entity::SetVelocityY(float mps)
	{
		b2Body* body = GetBox2DRigidbody();
		body->SetLinearVelocity({ body->GetLinearVelocity().x, mps });
	}

	glm::vec2 Entity::GetVelocity()
	{
		b2Vec2 velocity = GetBox2DRigidbody()->GetLinearVelocity();
		return glm::vec2{ velocity.x, velocity.y };
	}

	void Entity::ApplyImpulse(const glm::vec2& impulse)
	{
		b2Body* body = GetBox2DRigidbody();
		body->ApplyLinearImpulse({impulse.x, impulse.y }, body->GetWorldCenter(), true);
	}

	void Entity::DeleteAllScriptInstances()
	{
		for (auto& [scriptName, scriptInstance] : GetComponent<ScriptComponent>().Scripts)
		{
			if (m_Scene->m_SceneState != SceneState::Edit)
				scriptInstance->OnDestroy();

			delete scriptInstance;
			scriptInstance = nullptr;
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

		m_Scene->DestroyEntity(*this);
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