#pragma once

enum PlatformState : uint16_t
{
	Normal = 0,

};

class Platform : public EntityScript
{
public:
	ENTITY_SCRIPT_CLASS(Platform)

	void OnRegisterFields() override;
	bool OnCreate() override;
	void OnUpdate(float ts) override;

private:
	float m_VanishAfter = 0.33f;
	float m_VanishTime = 1.0f;

	bool m_EnableCollision = true;
	bool m_ContactWithPlayer = false;
	float m_VanishTimer = 0.0f;
};