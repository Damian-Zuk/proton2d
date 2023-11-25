#pragma once

enum PlayerDirection : bool
{
	Right = 0, Left = 1
};

enum PlayerAnimation : uint32_t
{
	Idle = 0, Run, Attack, Jump
};

class PlayerScript: public proton::EntityScript
{
public:
	ENTITY_SCRIPT_CLASS(PlayerScript)

	virtual void OnRegisterFields() override;
	virtual void OnCreate() override;
	virtual void OnUpdate(float ts) override;

private:
	float m_PlayerSpeed = 5.0f;
	float m_JumpForce = 5.0f;

	proton::Shared<proton::SpriteAnimation> m_Animation;
	PlayerDirection m_Direction = Right;

	bool m_IsAttacking = false;
	bool m_IsJumping = true;
	float m_JumpDelay = 0.0f;

	b2Body* m_Body = nullptr;
	proton::Entity m_FootSensor;
	uint32_t m_ContactCount = 0;
};
