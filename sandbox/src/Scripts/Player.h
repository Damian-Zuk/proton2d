#pragma once

enum PlayerState : uint16_t // animation index
{
	PlayerState_Idle = 0,
	PlayerState_Run  = 1,
	PlayerState_Jump = 2,
	PlayerState_Land = 3
};

struct PlayerInputState
{
	bool MoveLeft = false;
	bool MoveRight = false;
	bool Jump = false;

	bool operator==(const PlayerInputState& other) const 
	{
		return other.MoveLeft == MoveLeft
			&& other.MoveRight == MoveRight
			&& other.Jump == Jump;
	}

	bool operator!=(const PlayerInputState& other) const 
	{
		return !(other == *this);
	}
};

class Player : public EntityScript
{
public:
	ENTITY_SCRIPT_CLASS(Player)

	virtual void OnPreInit() override;
	virtual bool OnCreate() override;
	virtual void OnUpdate(float ts) override;
	virtual void OnFixedUpdate(float ts) override;
	virtual void OnImGuiRender();

	void SetPlayerColor(const glm::vec4& color);
	void SetPlayerNick(const std::string& nick);

private:
	bool IsOnHighSlope() const;

private:
	bool m_IsLocalPlayer = true;
	uint32_t m_ClientID = 0;
	
	float m_JumpImpulse = 27.0f;
	float m_FallModifier = 100.0f;
	float m_PlayerSpeed = 12.0f;
	float m_PlayerAcceleration = 100.0f;
	
	std::string m_PlayerNick;
	glm::vec4 m_PlayerColor { 1.0f };

	// Internal state
	b2Body* m_Body = nullptr;
	b2Body* m_Wheel = nullptr;

	PlayerInputState m_InputState;
	PlayerInputState m_PreviousInputState;
	PlayerState m_State = PlayerState_Idle;

	glm::vec2 m_LinearVelocity = { 0.0f, 0.0f };
	float m_Direction = 1.0f;
	float m_LastJumpTimer = 0.1f;
	float m_OriginalLinearDamping = 0.0f;
	float m_LinearDampingTimer = 0.0f;

	friend class MyGameMode;
};
