#include "pch.h" // todo: make own precompiled header for sandbox
#include "PlayerScript.h"

#include <proton/Core/Input.h>

#include <box2d/b2_contact.h>
#include <box2d/b2_world_callbacks.h>

using namespace proton;

ENTITY_SCRIPT_IMPLEMENTATION(PlayerScript);

enum PlayerAnimation : uint32_t
{
	Idle = 0, Run = 1, Attack = 2, Jump = 3
};

void PlayerScript::RegisterFields()
{
	RegisterField(ScriptFieldType::Float, "PlayerSpeed", &m_PlayerSpeed);
	RegisterField(ScriptFieldType::Float, "JumpForce", &m_JumpForce);
	RegisterField(ScriptFieldType::Float, "AnimationFrameTime", &m_AnimationFrameTime);
}

void PlayerScript::OnCreate()
{
	m_Body = GetBox2DRigidbody();
}

void PlayerScript::OnUpdate(float ts)
{
	auto& sprite = GetComponent<SpriteComponent>().Sprite;
	PlayerAnimation animation = PlayerAnimation::Idle;

	bool isAttacking = Input::IsKeyPressed(Key::Space);

	if (!isAttacking)
	{
		// Move left / right
		if (Input::IsKeyPressed(Key::Right))
		{
			SetVelocityX(m_PlayerSpeed);
			sprite->FlipX(false);
			animation = PlayerAnimation::Run;
		}
		else if (Input::IsKeyPressed(Key::Left))
		{
			SetVelocityX(-m_PlayerSpeed);
			sprite->FlipX(true);
			animation = PlayerAnimation::Run;
		}
		else
			SetVelocityX(0.0f);

		// Jump
		if (Input::IsKeyPressed(Key::Up))
		{
			if (GetVelocity().y == 0.0f)
				ApplyImpulse({ 0.0f,  m_JumpForce });
		}
	}
	else
	{
		animation = PlayerAnimation::Attack;
		SetVelocityX(0.0f);
	}

	// Update animation
	if (m_AnimationTimer.OnInterval(m_AnimationFrameTime))
	{
		sprite->NextTile(animation);
	}

	m_AnimationTimer.Tick(ts);
}