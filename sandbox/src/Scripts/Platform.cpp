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

	collider.ContactCallback.OnBegin = [&](PhysicsContact contact) {
		if (m_EnableCollision && contact.Other->GetTag() == "Sensor_Bottom")
		{
			m_VanishTimer = 0.0f;
			m_ContactWithPlayer = true;
		}
	};

	collider.ContactCallback.OnPreSolve = [&](PhysicsContact contact, const b2Manifold* oldManifold) {
		contact.Contact->SetEnabled(m_EnableCollision);
	};

	return true;
}

void Platform::OnUpdate(float ts)
{
	if (m_ContactWithPlayer)
	{
		if (m_EnableCollision && m_VanishTimer > m_VanishAfter)
		{
			m_EnableCollision = false;
			GetComponent<ResizableSpriteComponent>().Color.a = 0.33f;
		}
		if (!m_EnableCollision && m_VanishTimer > m_VanishAfter + m_VanishTime)
		{
			m_EnableCollision = true;
			m_ContactWithPlayer = false;
			GetComponent<ResizableSpriteComponent>().Color.a = 1.0f;
		}

		m_VanishTimer += ts;
	}
}
