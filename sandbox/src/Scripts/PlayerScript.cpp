#include "pch.h" // todo: make own precompiled header for sandbox
#include "PlayerScript.h"

#include <proton/Core/Input.h>

#include <box2d/b2_contact.h>
#include <box2d/b2_world_callbacks.h>

using namespace proton;

ENTITY_SCRIPT_IMPLEMENTATION(PlayerScript);

void PlayerScript::RegisterFields()
{
	RegisterField(ScriptFieldType::Float, "PlayerSpeed", &m_PlayerSpeed);
	RegisterField(ScriptFieldType::Float, "JumpForce", &m_JumpForce);
	RegisterField(ScriptFieldType::Float, "Threshold", &threshold);
}

void PlayerScript::OnCreate()
{
	m_Body = GetBox2DRigidbody();
	// Create animations flipbook
	m_Flipbook = AddComponent<FlipbookAnimationComponent>().Flipbook;
	m_Flipbook->SetFPS(10);
	m_Flipbook->CreateAnimation(Idle, 10);
	m_Flipbook->CreateAnimation(Run, 10);
	m_Flipbook->CreateAnimation(Attack, 10);
	m_Flipbook->CreateAnimation(Jump, 3);
	m_Flipbook->SetAnimation(Idle, Right);
	
}

void PlayerScript::OnUpdate(float ts)
{
	// Attack
	if (m_IsAttacking)
	{
		bool space = Input::IsKeyPressed(Key::Space);
		if (!space && m_Flipbook->GetProgress() >= 0.5f)
		{
			// Stop attacking
			m_Flipbook->SetAnimation(Idle, m_Orientation);
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
			m_Flipbook->SetAnimation(Attack, m_Orientation);
			m_Flipbook->SetPlayMode(FlipbookPlayMode::PLAY_ONCE);
			m_Flipbook->Replay();
			m_IsAttacking = true;
		}
	} 

	// Movement
	if (Input::IsKeyPressed(Key::Right))
	{
		m_Orientation = Right;
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
	else if (Input::IsKeyPressed(Key::Left))
	{
		m_Orientation = Left;
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
			m_Flipbook->SetAnimation(Idle, m_Orientation);
	}

	// Jumping
	if (Input::IsKeyPressed(Key::Up))
	{
		if (GetVelocity().y == 0.0f) 
			ApplyImpulse({ 0.0f,  m_JumpForce });
	}	

}