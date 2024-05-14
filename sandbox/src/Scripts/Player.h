#pragma once

enum PlayerState : uint16_t // animation index
{
	PlayerState_Idle = 0,
	PlayerState_Run  = 1,
	PlayerState_Jump = 2,
	PlayerState_Land = 3
};

struct PlayerActionState
{
	bool MoveLeft = false;
	bool MoveRight = false;
	bool Jump = false;

	bool operator==(const PlayerActionState& other) const 
	{
		return other.MoveLeft == MoveLeft
			&& other.MoveRight == MoveRight
			&& other.Jump == Jump;
	}

	bool operator!=(const PlayerActionState& other) const 
	{
		return !(other == *this);
	}
};

class Player : public EntityScript
{
public:
	ENTITY_SCRIPT_CLASS(Player)

	virtual void OnRegisterFields() override;
	virtual bool OnCreate() override;
	virtual void OnUpdate(float ts) override;
	virtual void OnPhysicsUpdate(float ts) override;
	virtual void OnImGuiRender();

private:
	bool IsGrounded() const;
	bool IsOnHighSlope() const;

private:
	bool m_IsLocalPlayer = true;
	uint32_t m_ClientID = 0;

	b2Body* m_Wheel = nullptr;

	PlayerActionState m_ActionState;
	float m_PlayerMaxSpeed = 6.0f;
	float m_JumpForce = 20.0f;
	float m_PlayerAcceleration = 40.0f;
	float m_GravityModifier = -10.0f;

	PlayerState m_State = PlayerState_Idle;
	float m_Direction = 1.0f;
	float m_JumpTimer = 0.0f;

	friend class MyGameMode;
};
