#pragma once

enum PlayerState : uint16_t
{
	Idle, Run, Jump, Land
};

struct PlayerActionState
{
	bool MoveLeft = false;
	bool MoveRight = false;
	bool Jump = false;

	bool operator==(const PlayerActionState& other) const {
		return other.MoveLeft == MoveLeft
			&& other.MoveRight == MoveRight
			&& other.Jump == Jump;
	}
	bool operator!=(const PlayerActionState& other) const {
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

	PlayerActionState m_ActionState;

	float m_PlayerMaxSpeed = 6.0f;
	float m_JumpForce = 20.0f;
	float m_PlayerAcceleration = 40.0f;
	float m_GravityModifier = -10.0f;

	b2Body* m_Wheel = nullptr;
	PlayerState m_State = Idle;
	float m_Direction = 1.0f;
	float m_JumpTimer = 0.0f;

	friend class MyGameMode;
};
