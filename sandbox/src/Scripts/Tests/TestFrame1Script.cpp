#include <Proton.h>
using namespace proton;

#include "TestFrame1Script.h"

void TestFrame1Script::OnRegisterFields()
{
	REGISTER_FIELD(Float, m_BounceForce);
	REGISTER_FIELD(Float, m_Interval);
}

bool TestFrame1Script::OnCreate()
{
	m_Bouncer = FindChildByTag("Bouncer");
	return true;
}

void TestFrame1Script::OnUpdate(float ts)
{
	if (IsRunningClient())
		return;

	if (m_Timer > m_Interval)
	{
		b2Body* body = m_Bouncer.GetRuntimeBody();
		body->ApplyLinearImpulseToCenter({ 0.0f, m_BounceForce }, true);
		m_Timer = 0.0f;
		return;
	}
	m_Timer += ts;
}
