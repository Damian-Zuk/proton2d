#pragma once

#include <proton/Scripting.h>

using namespace proton;

class PlayerRunRight: public EntityScript
{
public:
	virtual void OnCreate() override
	{
		GetComponent<SpriteComponent>().Sprite->SetTile(0, 1);
	}

	virtual void OnUpdate(float ts) override
	{
		auto& transform = GetComponent<TransformComponent>();
		transform.Position.x += 0.1f * ts;

		m_SpriteChangeTime -= ts;
		if (m_SpriteChangeTime < 0.0f)
		{
			m_SpriteChangeTime = m_SpriteChangeTimeBase;
			GetComponent<SpriteComponent>().Sprite->NextTile();
		}
	}
private:
	float m_SpriteChangeTimeBase = 0.1f;
	float m_SpriteChangeTime = m_SpriteChangeTimeBase;
};