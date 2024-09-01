#pragma once

class DynamicExtrapolation : public EntityScript
{
public:
	ENTITY_SCRIPT_CLASS(DynamicExtrapolation)

	virtual void OnRegisterFields() override;
	virtual void OnUpdate(float ts) override;

private:
	float m_Range = 2.0f;

	float m_Cooldown = 0.5f;
	float m_CooldownTimer = 0.0f;
	Entity m_LocalPlayer;
};