#include "pch.h" // TODO: REMOVE
#include "PlayerScript.h"

#include <proton/Core/Input.h>

using namespace proton;

enum PlayerAnimation : uint32_t
{
	Idle = 0, Run = 1, Attack = 2
};

void PlayerScript::OnUpdate(float ts)
{
	auto& position = GetComponent<TransformComponent>().Position;
	auto& sprite = GetComponent<SpriteComponent>().Sprite;
	glm::vec3 positionBeforeInput = position;

	// User input
	bool attacking = Input::IsKeyPressed(Key::Space);

	if (!attacking)
	{
		if (Input::IsKeyPressed(Key::Right))
		{
			position.x += m_PlayerSpeed * ts;
			sprite->FlipX(false);
		}

		if (Input::IsKeyPressed(Key::Left))
		{
			position.x -= m_PlayerSpeed * ts;
			sprite->FlipX(true);
		}
	}

	// Update animation
	if (m_AnimationTimer.OnInterval(m_AnimationFrameTime))
	{
		if (attacking)
		{
			sprite->NextTile(PlayerAnimation::Attack);
		}
		else
		{
			if (position != positionBeforeInput)
				sprite->NextTile(PlayerAnimation::Run);
			else
				sprite->NextTile(PlayerAnimation::Idle);
		}
	}

	m_AnimationTimer.Tick(ts);
}