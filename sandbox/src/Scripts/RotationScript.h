#pragma once

#include <Proton2D.h>

using namespace proton;

class RotationScript : public proton::EntityScript
{
public:
	ENTITY_SCRIPT_CLASS(RotationScript)

	virtual void RegisterFields()
	{
		RegisterField(proton::ScriptFieldType::Float, "RotationSpeed", &m_RotationSpeed);
	}

	virtual void OnCreate()
	{
		m_Body = GetBox2DRigidbody();
	}

	virtual void OnUpdate(float ts) override
	{
		if (m_Body) 
		{
			m_Body->SetTransform(m_Body->GetPosition(),
				m_Body->GetAngle() + m_RotationSpeed * b2_pi * ts);
		}
		else
			GetTransform().Rotation += m_RotationSpeed;
	}

private:
	b2Body* m_Body = nullptr;
	float m_RotationSpeed = 0.5f;
};
