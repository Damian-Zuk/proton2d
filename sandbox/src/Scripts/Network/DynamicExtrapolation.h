#pragma once

class DynamicExtrapolation : public EntityScript
{
public:
	ENTITY_SCRIPT_CLASS(DynamicExtrapolation)

	virtual void OnPreInit() override;
	virtual bool OnCreate() override;
	virtual void OnFixedUpdate(float ts) override;

private:
	void OnBeginPhysicsContact(const PhysicsContact& contact);

private:
	float InterpolationThreshold = 0.05f;
	float SwitchCooldownTime = 0.25f;
	bool Enable = true;
	bool AlphaVisualize = false;

	float m_PredictionTimer = 0.0f;

	friend class MyGameMode;
};