#include <Proton.h>
using namespace proton;

#include "Platform.h"

void Platform::OnRegisterFields()
{
	RegisterField(ScriptFieldType::Float, "VanishAfter", &m_VanishAfter);
	RegisterField(ScriptFieldType::Float, "VanishTime", &m_VanishTime);
}

bool Platform::OnCreate()
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

void Platform::OnUpdate(float ts)
{

	if (m_EnableCollision)

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
			GetComponent<SpriteComponent>().Color.a = 0.33f;
		}

		// Appear the platform (enable collision)
		if (!m_EnableCollision && m_VanishTimer > m_VanishAfter + m_VanishTime)
		{
			m_EnableCollision = true;
			m_ContactWithPlayer = false;
			GetComponent<SpriteComponent>().Color.a = 1.0f;
		}

		m_VanishTimer += ts;
	}
}
