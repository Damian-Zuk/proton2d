#include <Proton.h>
using namespace proton;

#include "Platform.h"

void RedPlatform::OnRegisterFields()
{
	REGISTER_FIELD(Float, m_VanishAfter);
	REGISTER_FIELD(Float, m_VanishTime);
}

bool RedPlatform::OnCreate()
{
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
		
		if (contact.Other->GetTag() == "Sensor_Bottom")
		{
			// Manually decrement player bottom sensor contact count
			// TODO: Do this in PhysicsWorld callback
			m_PlayerSensor = *contact.Other;
			m_PlayerSensor.GetComponent<BoxColliderComponent>().ContactCallback.ContactCount--;
			m_DecrementedSensor = true;
		}
	};

	return true;
}

void RedPlatform::OnUpdate(float ts)
{
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
