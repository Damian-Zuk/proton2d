#pragma once

#include <proton/Core/Timer.h>
#include <proton/Scripting.h>

class PlayerScript: public proton::EntityScript
{
public:
	virtual void OnUpdate(float ts) override;
private:
	proton::Timer m_AnimationTimer;
	float m_PlayerSpeed = 0.7f;
};