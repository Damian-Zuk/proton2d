#include "pch.h" // todo: make own precompiled header for sandbox
#include "PlayerScript.h"

#include <proton/Core/Input.h>

#include <box2d/b2_contact.h>
#include <box2d/b2_world_callbacks.h>

#include <glm/gtc/type_ptr.hpp>

using namespace proton;

void PlayerScript::OnRegisterFields()
{
	RegisterField(ScriptFieldType::Float, "PlayerSpeed", &m_PlayerSpeed);
	RegisterField(ScriptFieldType::Float, "JumpForce", &m_JumpForce);
}

void PlayerScript::OnCreate()
{
	m_Body = GetBox2DRigidbody();
	// Create animations flipbook
	m_Flipbook = &AddComponent<FlipbookAnimationComponent>().Flipbook;
	m_Flipbook->SetFPS(10);
	m_Flipbook->CreateAnimation(Idle, 10);
	m_Flipbook->CreateAnimation(Run, 10);
	m_Flipbook->CreateAnimation(Attack, 10);
	m_Flipbook->CreateAnimation(Jump, 3);
	m_Flipbook->SetAnimation(Idle, Right);

	m_FootSensor = GetScene()->FindByTag("FootSensor");
	if (m_FootSensor)
	{
		auto& bc = m_FootSensor.GetComponent<BoxColliderComponent>();

		bc.ContactCallback.OnBeginContactFunction = [&](PhysicsContactInfo info)
		{
			m_ContactCount++;
			LOG("BeginContact", info.OtherUUID);
		};

		bc.ContactCallback.OnEndContactFunction = [&](PhysicsContactInfo info)
		{
			m_ContactCount--;
			LOG("EndContact", info.OtherUUID);
		};
	}
}

void PlayerScript::OnUpdate(float ts)
{
	auto& position = GetTransform().Position;
	if (m_FootSensor)
	{
		b2Body* sensorBody = m_FootSensor.GetBox2DRigidbody();
		sensorBody->SetTransform({ position.x, position.y }, 0.0f);
		sensorBody->SetLinearVelocity({0.0f, 0.0f});
	}

	// Attack
	if (m_IsAttacking)
	{
		bool space = Input::IsKeyPressed(Key::Space);
		if (!space && m_Flipbook->GetProgress() >= 0.5f)
		{
			// Stop attacking
			m_Flipbook->SetAnimation(Idle, m_Direction);
			m_Flipbook->SetPlayMode(FlipbookPlayMode::REPEAT);
			m_IsAttacking = false;
		}
		else if (space && m_Flipbook->FinishedPlaying())
			m_Flipbook->Replay();
	}
	if (!m_IsAttacking)
	{
		if (Input::IsKeyPressed(Key::Space))
		{
			// Begin attacking
			m_Flipbook->SetAnimation(Attack, m_Direction);
			m_Flipbook->SetPlayMode(FlipbookPlayMode::PLAY_ONCE);
			m_Flipbook->Replay();
			m_IsAttacking = true;
		}
	} 

	// Movement
	if (Input::IsKeyPressed(Key::D))
	{
		m_Direction = Right;
		if (m_IsAttacking) // Walking while attacking
		{
			SetVelocityX(m_PlayerSpeed / 2);
			m_Flipbook->SetMirrorFlip(Right);
		}
		else // Running
		{
			SetVelocityX(m_PlayerSpeed);
			m_Flipbook->SetAnimation(Run, Right);
		}
	}
	else if (Input::IsKeyPressed(Key::A))
	{
		m_Direction = Left;
		if (m_IsAttacking) // Walking while attacking
		{
			SetVelocityX(-m_PlayerSpeed / 2);
			m_Flipbook->SetMirrorFlip(Left);
		}
		else // Running
		{
			SetVelocityX(-m_PlayerSpeed);
			m_Flipbook->SetAnimation(Run, Left);
		}
	}
	else  
	{
		SetVelocityX(0.0f);
		if (!m_IsAttacking) // Idle
			m_Flipbook->SetAnimation(Idle, m_Direction);
	}

	// Jumping
	if (Input::IsKeyPressed(Key::W))
	{
		if (m_ContactCount > 0)
		{
			ApplyImpulse({ 0.0f,  m_JumpForce });
		}
	}	
}
