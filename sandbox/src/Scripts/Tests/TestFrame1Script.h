#pragma once

class TestFrame1Script : public EntityScript
{
public:
	ENTITY_SCRIPT_CLASS(TestFrame1Script)

	virtual void OnPreInit() override;
	virtual bool OnCreate() override;
	virtual void OnUpdate(float ts) override;
private:
	float m_BounceForce = 10.0f;
	float m_Interval = 0.5f;

	Entity m_Bouncer;
	float m_Timer = 0.0f;
};