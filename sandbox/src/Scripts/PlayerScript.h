#pragma once

#include <proton/Scene/EntityScript.h>
#include <proton/Graphics/SpriteAnimation.h>
#include <proton/Core/UUID.h>

#include <box2d/include/box2d/b2_body.h>

using namespace proton;

enum PlayerDirection : bool
{
	Right = 0, Left = 1
};

enum PlayerAnimation : uint32_t
{
	Idle = 0, Run, Attack, Jump
};

class PlayerScript: public EntityScript
{
public:
	ENTITY_SCRIPT_CLASS(PlayerScript)

	virtual void OnRegisterFields() override;
	virtual void OnCreate() override;
	virtual void OnUpdate(float ts) override;

private:
	float m_PlayerSpeed = 5.0f;
	float m_JumpForce = 5.0f;

	Shared<SpriteAnimation> m_Animation;
	PlayerDirection m_Direction = Right;

	bool m_IsAttacking = false;
	bool m_IsJumping = true;
	float m_JumpThreshold = 0.0f;

	b2Body* m_Body = nullptr;
	Entity m_FootSensor;
	uint32_t m_ContactCount = 0;
};