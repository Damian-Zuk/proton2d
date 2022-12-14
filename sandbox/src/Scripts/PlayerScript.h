#pragma once

#include <proton/Core/Timer.h>
#include <proton/Scripting.h>
#include <box2d/include/box2d/b2_body.h>

class PlayerScript: public proton::EntityScript
{
public:
	virtual void RegisterFields() override;
	virtual void OnCreate() override;
	virtual void OnUpdate(float ts) override;
private:
	proton::Timer m_AnimationTimer;
	float m_AnimationFrameTime = 0.08f;
	float m_PlayerSpeed = 5.0f;
	float m_JumpForce = 5.0f;

	b2Body* m_Body = nullptr;
};