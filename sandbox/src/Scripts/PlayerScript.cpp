#include "pch.h" // todo: make own precompiled header for sandbox
#include "PlayerScript.h"

#include <proton/Core/Input.h>

#include <box2d/b2_contact.h>
#include <box2d/b2_world_callbacks.h>

using namespace proton;

enum PlayerAnimation : uint32_t
{
	Idle = 0, Run = 1, Attack = 2, Jump = 3
};

void PlayerScript::RegisterFields()
{
	RegisterFloatField("PlayerSpeed", &m_PlayerSpeed);
	RegisterFloatField("JumpForce", &m_JumpForce);
	RegisterFloatField("AnimationFrameTime", &m_AnimationFrameTime);
}

void PlayerScript::OnCreate()
{
	m_Body = GetBox2DRigidBody();
}

void PlayerScript::OnUpdate(float ts)
{
	auto& sprite = GetComponent<SpriteComponent>().Sprite;
	bool attacking = Input::IsKeyPressed(Key::Space);

	PlayerAnimation animation = PlayerAnimation::Idle;

	if (!attacking)
	{
		// Move left / right
		if (Input::IsKeyPressed(Key::Right))
		{
			m_Body->SetLinearVelocity({ m_PlayerSpeed, m_Body->GetLinearVelocity().y });
			sprite->FlipX(false);
			animation = PlayerAnimation::Run;
		}
		else if (Input::IsKeyPressed(Key::Left))
		{
			m_Body->SetLinearVelocity({ -m_PlayerSpeed, m_Body->GetLinearVelocity().y });
			sprite->FlipX(true);
			animation = PlayerAnimation::Run;
		}
		else
			m_Body->SetLinearVelocity({ 0.0f, m_Body->GetLinearVelocity().y });

		// Jump
		if (Input::IsKeyPressed(Key::Up))
		{
			if (m_Body->GetLinearVelocity().y == 0.0f)
				m_Body->ApplyLinearImpulse({ 0.0f, m_Body->GetMass() * m_JumpForce }, m_Body->GetWorldCenter(), true);
		}
	}
	else
	{
		animation = PlayerAnimation::Attack;
		m_Body->SetLinearVelocity({ 0.0f, m_Body->GetLinearVelocity().y });
	}

	// Update animation
	if (m_AnimationTimer.OnInterval(m_AnimationFrameTime))
	{
		sprite->NextTile(animation);
	}

	m_AnimationTimer.Tick(ts);
}