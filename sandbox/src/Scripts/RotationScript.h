// Header-only script example
#pragma once

class RotationScript : public EntityScript
{
public:
	ENTITY_SCRIPT_CLASS(RotationScript)

	virtual void OnPreInit() override 
	{
		REGISTER_FIELD(Float, m_RotationSpeed);
	}

	virtual void OnUpdate(float ts) override 
	{
		RotateCenter(m_RotationSpeed * ts);
	}

private:
	float m_RotationSpeed = 1.0f;
};

