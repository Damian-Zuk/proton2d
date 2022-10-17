#include "pch.h" // TODO: REMOVE
#include "PlayerScript.h"

#include <proton/Core/Input.h>

using namespace proton;

enum PlayerAnimation : uint32_t
{
	Idle = 0, Run = 1
};

void PlayerScript::OnUpdate(float ts)
{
	auto& position = GetComponent<TransformComponent>().Position;
	auto& sprite = GetComponent<SpriteComponent>().Sprite;
	glm::vec3 positionBeforeInput = position;

	// Update input
	if (Input::IsKeyPressed(Key::Right))
		position.x += m_PlayerSpeed * ts;

	if (Input::IsKeyPressed(Key::Left))
		position.x -= m_PlayerSpeed * ts;

	// Update animation
	if (m_AnimationTimer.OnInterval(1.0f / 8.0f))
	{
		if (position != positionBeforeInput)
			sprite->NextTile(PlayerAnimation::Run);
		else
			sprite->NextTile(PlayerAnimation::Idle);
	}

	m_AnimationTimer.Tick(ts);
}