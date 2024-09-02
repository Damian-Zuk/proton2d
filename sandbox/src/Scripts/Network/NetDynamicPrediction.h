#pragma once

class NetDynamicPrediction : public EntityScript
{
public:
	ENTITY_SCRIPT_CLASS(NetDynamicPrediction)

	virtual void OnRegisterFields() override;
	virtual bool OnCreate() override;
	virtual void OnUpdate(float ts) override;

private:
	void OnBeginPhysicsContact(const PhysicsContact& contact);

private:
	float PredictionTime = 3.0f;
	float SwitchCooldown = 0.5f;

	float m_PredictionTimer = 0.0f;
	float m_CooldownTimer = 0.0f;
};