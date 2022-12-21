#pragma once

#include <proton/Scripting.h>
#include <box2d/include/box2d/b2_body.h>

using namespace proton;

class RotationScript : public EntityScript
{
public:
	ENTITY_SCRIPT_CLASS(RotationScript)

	virtual void RegisterFields()
	{
		RegisterField(ScriptFieldType::Float, "RotationSpeed", &m_RotationSpeed);
	}

	virtual void OnCreate()
	{
		m_Body = GetBox2DRigidbody();
	}

	virtual void OnUpdate(float ts) override
	{
		m_Body->SetTransform(m_Body->GetPosition(),
			m_Body->GetAngle() + m_RotationSpeed * b2_pi * ts);
	}

private:
	b2Body* m_Body = nullptr;
	float m_RotationSpeed = 0.5f;
};