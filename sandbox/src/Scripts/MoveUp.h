#pragma once

#include <proton/Scripting.h>

using namespace proton;

class MoveUp : public EntityScript
{
public:
	virtual void OnUpdate(float ts) override
	{
		auto& transform = GetComponent<TransformComponent>();
		transform.Position.y += 0.05f * ts;
	}
};