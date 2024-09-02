#include <Proton.h>
using namespace proton;

#include "TestFrame2Script.h"

void TestFrame2Script::OnRegisterFields()
{
	REGISTER_FIELD(Float, m_Interval);
}

bool TestFrame2Script::OnCreate()
{
	if (IsNetModeClient())
		return true;

	m_BallSpawn = FindChildByTag("BallSpawn");
	m_BallGroup = FindChildByTag("BallGroup");
	Entity ballDespawn = FindChildByTag("BallDespawn");

	auto& bc = ballDespawn.GetComponent<BoxColliderComponent>();
	bc.ContactCallback.OnBegin = [&](PhysicsContact contact) {
		if (contact.Other->GetTag() == "Ball")
			m_BallToDestroy = *contact.Other;
	};

	return true;
}

void TestFrame2Script::OnUpdate(float ts)
{
	if (IsNetModeClient())
		return;

	if (m_BallToDestroy)
	{
		m_BallToDestroy.Destroy();
		m_BallToDestroy = Entity();
	}

	if (m_Timer >= m_Interval)
	{
		auto& transform = m_BallSpawn.GetTransform();
		Entity ball = PrefabManager::Spawn(GetScene(), "Ball");
		ball.SetWorldPosition({ transform.WorldPosition.x + Random::Float(-0.5f, 0.5f), transform.WorldPosition.y});
		m_BallGroup.AddChildEntity(ball);
		m_Timer = Random::Float(0.0f, 0.2f);
		return;
	}
	m_Timer += ts;
}
