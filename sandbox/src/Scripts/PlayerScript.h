#pragma once

#include <proton/Core/Timer.h>
#include <proton/Scripting.h>

class PlayerScript: public proton::EntityScript
{
public:
	virtual void OnUpdate(float ts) override;
private:
	proton::Timer m_AnimationTimer;
	float m_AnimationFrameTime = 0.08f;
	float m_PlayerSpeed = 5.0f;
};