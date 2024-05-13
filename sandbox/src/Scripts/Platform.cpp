#include <Proton.h>
using namespace proton;

#include "Platform.h"

static void OnRep_EnableCollision(Entity* entity)
{
	RedPlatform* platform = entity->As<RedPlatform>();
	_PT_TRACE("OnRep_EnableCollision: {}", platform->m_EnableCollision);

	if (platform->m_EnableCollision)
		platform->GetComponent<ResizableSpriteComponent>().Color.a = 1.0f;
	else
		platform->GetComponent<ResizableSpriteComponent>().Color.a = 0.33f;
}

void RedPlatform::OnRegisterFields()
{
	RegisterField(ScriptFieldType::Float, "VanishAfter", &m_VanishAfter);
	RegisterField(ScriptFieldType::Float, "VanishTime", &m_VanishTime);

	PT_REPLICATE_DATA(m_EnableCollision, &OnRep_EnableCollision);
}

bool RedPlatform::OnCreate()
{
	if (IsRunningClient())
		return true;

	auto& collider = GetComponent<BoxColliderComponent>();

	// OnBegin Contact Callback
	collider.ContactCallback.OnBegin = [&](PhysicsContact contact) {
		if (contact.Other->GetTag() == "Sensor_Bottom")
		{
			if (m_EnableCollision)
			{
				m_VanishTimer = 0.0f;
				m_ContactWithPlayer = true;
				m_PlayerSensor = *contact.Other;
				return;
			}

			// Manually decrement player bottom sensor contact count
			m_PlayerSensor = *contact.Other;
			m_PlayerSensor.GetComponent<BoxColliderComponent>().ContactCallback.ContactCount--;
			m_DecrementedSensor = true;
		}
	};

	// OnEnd Contact Callback
	collider.ContactCallback.OnEnd = [&](PhysicsContact contact) {
		if (m_PlayerSensor && contact.Other->GetTag() == "Sensor_Bottom")
		{
			if (m_DecrementedSensor)
			{
				// Restore sensor contact count
				m_PlayerSensor.GetComponent<BoxColliderComponent>().ContactCallback.ContactCount++;
				m_DecrementedSensor = false;
			}
			m_PlayerSensor = Entity();
		}
	};

	// OnPreSolve Contact Callback
	collider.ContactCallback.OnPreSolve = [&](PhysicsContact contact, const b2Manifold* oldManifold) {
		contact.Contact->SetEnabled(m_EnableCollision);
	};

	return true;
}

void RedPlatform::OnUpdate(float ts)
{
	if (IsRunningClient())
		return;

	if (m_ContactWithPlayer && m_VanishAfter > 0.0f)
	{
		// Vanish the platform (disable collision)
		if (m_EnableCollision && m_VanishTimer > m_VanishAfter)
		{
			m_EnableCollision = false;
			if (m_PlayerSensor)
			{
				// Manually decrement player bottom sensor contact count
				m_PlayerSensor.GetComponent<BoxColliderComponent>().ContactCallback.ContactCount--;
				m_DecrementedSensor = true;
			}
			GetComponent<ResizableSpriteComponent>().Color.a = 0.33f;
		}

		// Appear the platform (enable collision)
		if (!m_EnableCollision && m_VanishTimer > m_VanishAfter + m_VanishTime)
		{
			m_EnableCollision = true;
			m_ContactWithPlayer = false;
			GetComponent<ResizableSpriteComponent>().Color.a = 1.0f;
		}

		m_VanishTimer += ts;
	}
}
