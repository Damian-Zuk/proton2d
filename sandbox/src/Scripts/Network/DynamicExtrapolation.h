#pragma once

class DynamicExtrapolation : public EntityScript
{
public:
	ENTITY_SCRIPT_CLASS(DynamicExtrapolation)

	virtual void OnRegisterFields() override;
	virtual bool OnCreate() override;
	virtual void OnUpdate(float ts) override;

private:
	void OnBeginPhysicsContact(const PhysicsContact& contact);

private:
	float InterpolationThreshold = 0.05f;
	float SwitchCooldownTime = 0.25f;
	bool AlphaVisualize = false;

	float m_PredictionTimer = 0.0f;
};