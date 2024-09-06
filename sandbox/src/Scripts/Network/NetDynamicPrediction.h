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
	float InterpolationThreshold = 0.05f;
	float SwitchCooldown = 0.25f;
	bool AlphaVisualize = false;

	float m_PredictionTimer = 0.0f;
};