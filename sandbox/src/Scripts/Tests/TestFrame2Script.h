#pragma once

class TestFrame2Script : public EntityScript
{
public:
	ENTITY_SCRIPT_CLASS(TestFrame2Script)

	virtual void OnPreInit() override;
	virtual bool OnCreate() override;
	virtual void OnUpdate(float ts) override;

private:
	float m_Interval = 1.0f;

	Entity m_BallToDestroy;
	Entity m_BallSpawn;
	Entity m_BallGroup;
	float m_Timer = 0.0f;
};