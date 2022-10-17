#pragma once

#include <proton/Scripting.h>

using namespace proton;

class RotationScript : public EntityScript
{
public:
	virtual void OnUpdate(float ts) override
	{
		auto& transform = GetComponent<TransformComponent>().Rotation += 45.0f * ts;
	}
};