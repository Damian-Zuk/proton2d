#pragma once

class RedPlatform : public EntityScript
{
public:
	ENTITY_SCRIPT_CLASS(RedPlatform)

	void OnPreInit() override;
	bool OnCreate() override;
	void OnUpdate(float ts) override;

private:
	float m_VanishAfter = 0.33f;
	float m_VanishTime = 1.0f;

	bool m_EnableCollision = true;
	bool m_ContactWithPlayer = false;
	bool m_DecrementedSensor = false;
	float m_VanishTimer = 0.0f;

	Entity m_PlayerSensor;
};