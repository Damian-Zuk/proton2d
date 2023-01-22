#include "pcheader.h"
#include "PlayerScript.h"

using namespace proton;

void PlayerScript::OnRegisterFields()
{
	RegisterField(ScriptFieldType::Float, "PlayerSpeed", &m_PlayerSpeed);
	RegisterField(ScriptFieldType::Float, "JumpForce", &m_JumpForce);
}

void PlayerScript::OnCreate()
{
	m_Body = GetBox2DRigidbody();
	// Create animations
	m_Animation = AddComponent<SpriteAnimationComponent>().SpriteAnimation;
	m_Animation->SetFPS(10);
	m_Animation->AddAnimation(Idle, 10);
	m_Animation->AddAnimation(Run, 10);
	m_Animation->AddAnimation(Attack, 10);
	m_Animation->AddAnimation(Jump, 3);
	m_Animation->SetAnimation(Idle, Right);

	proton::UUID playerUUID = m_Entity.GetUUID();

	m_FootSensor = GetScene()->FindByTag("FootSensor");
	if (m_FootSensor)
	{
		auto& bc = m_FootSensor.GetComponent<BoxColliderComponent>();

		bc.ContactCallback.OnBeginContactFunction = [&, playerUUID](PhysicsContactInfo info)
		{
			if (info.OtherUUID != playerUUID)
			{
				LOG("Begin", info.OtherUUID);
				m_ContactCount++;
			}
		};

		bc.ContactCallback.OnEndContactFunction = [&, playerUUID](PhysicsContactInfo info)
		{
			if (info.OtherUUID != playerUUID && m_ContactCount > 0)
			{
				LOG("End", info.OtherUUID);
				m_ContactCount--;
			}
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
		if (!space && m_Animation->GetProgress() >= 0.5f)
		{
			// Stop attacking
			m_Animation->StartAnimation(Idle, m_Direction);
			m_Animation->SetPlayMode(AnimationPlayMode::REPEAT);
			m_IsAttacking = false;
		}
		else if (space && m_Animation->FinishedPlaying())
			m_Animation->Replay();
	}
	if (!m_IsAttacking)
	{
		if (!m_IsJumping && Input::IsKeyPressed(Key::Space))
		{
			// Begin attacking
			m_Animation->StartAnimation(Attack, m_Direction);
			m_Animation->SetPlayMode(AnimationPlayMode::PLAY_ONCE);
			m_Animation->Replay();
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
			m_Animation->SetMirrorFlip(false);
		}
		else // Running
		{
			SetVelocityX(m_PlayerSpeed);
			if (!m_IsJumping)
				m_Animation->StartAnimation(Run);
			m_Animation->SetMirrorFlip(false);
		}
	}
	else if (Input::IsKeyPressed(Key::A))
	{
		m_Direction = Left;
		if (m_IsAttacking) // Walking while attacking
		{
			SetVelocityX(-m_PlayerSpeed / 2);
			m_Animation->SetMirrorFlip(true);
		}
		else // Running
		{
			SetVelocityX(-m_PlayerSpeed);
			if (!m_IsJumping)
				m_Animation->StartAnimation(Run);
			m_Animation->SetMirrorFlip(true);
		}
	}
	else  
	{
		SetVelocityX(0.0f);
		if (!m_IsAttacking && !m_IsJumping) // Idle
			m_Animation->SetAnimation(Idle, m_Direction);
	}

	// Jumping
	if (!m_IsAttacking && Input::IsKeyPressed(Key::W))
	{
		if (m_ContactCount > 0 && m_JumpThreshold == 0.0f)
		{
			m_IsJumping = true;
			ApplyImpulse({ 0.0f,  m_JumpForce });
			m_JumpThreshold = 0.5f;
		}
	}
	if (m_IsJumping || m_ContactCount == 0)
	{
		m_Animation->SetAnimation(Jump, m_Direction);
		m_IsJumping = true;
	}
	if (m_IsJumping && m_JumpThreshold == 0.0f && m_ContactCount > 0)
	{
		m_Animation->SetAnimation(Idle, m_Direction);
		m_IsJumping = false;
	}

	m_JumpThreshold = std::max(m_JumpThreshold - ts, 0.0f);
}
