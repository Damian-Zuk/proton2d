#include <Proton.h>
using namespace proton;

#include "Player.h"
#include "MyGameMode.h"

// Internal script parameters
static constexpr float s_JumpDelay = 0.2f;
static constexpr float s_JumpFrameSwitchTime = 0.3f;
static constexpr float s_LandAnimationDelay = 0.5f;
static constexpr float s_LandAnimationCancelTime = 0.2f;

enum SensorType : uint32_t
{
	Sensor_Left = 0,
	Sensor_Right,
	Sensor_BottomLeft,
	Sensor_BottomRight,
	Sensor_Bottom,
	Sensor_GroundDetector,
};

bool Player::IsGrounded() const
{
	return CheckSensor(Sensor_Bottom);
}

bool Player::OnCreate()
{
	uint32_t localPlayerID = GetGameMode<MyGameMode>()->GetLocalPlayerID();
	m_IsLocalPlayer = m_ClientID == localPlayerID;

	if (m_IsLocalPlayer)
		GetScene()->SetPrimaryCameraEntity(*this);

	if (IsRunningClient())
	{
		PT_TRACE("ClientID={}, IsLocalPlayer={}", m_ClientID, m_IsLocalPlayer);
		return true;
	}

	if (IsRunningServer())
	{
		GetGameModeBase()->Server_SetPlayerActionCallback(m_ClientID, [&](BufferStreamReader& stream) {
			stream.ReadRaw(m_ActionState);
		});
	}

	// Set up sprite animations
	AddComponent<SpriteAnimationComponent>();
	SpriteAnimation& animation = GetSpriteAnimation();
	animation.SetFPS(8);
	animation.Add(Idle, 10, AnimationPlayMode::REPEAT);
	animation.Add(Run, 8, AnimationPlayMode::REPEAT);
	animation.Add(Jump, 3, AnimationPlayMode::PAUSED);
	animation.Add(Land, 9, AnimationPlayMode::PLAY_ONCE);

	// Set physics sensors to following child entities
	SetPhysicsSensor(Sensor_GroundDetector, "Sensor_GroundDetector");
	SetPhysicsSensor(Sensor_BottomLeft, "Sensor_BottomLeft");
	SetPhysicsSensor(Sensor_BottomRight, "Sensor_BottomRight");
	SetPhysicsSensor(Sensor_Bottom, "Sensor_Bottom");

	return true;
}

bool Player::IsOnHighSlope() const
{
	return !CheckSensor(Sensor_Bottom) && (CheckSensor(Sensor_BottomLeft) || CheckSensor(Sensor_BottomRight));
}

void Player::OnUpdate(float ts)
{
	if (m_IsLocalPlayer)
	{
		PlayerActionState previous = m_ActionState;
		m_ActionState.MoveRight = Input::IsKeyPressed(Key::D, this);
		m_ActionState.MoveLeft = Input::IsKeyPressed(Key::A, this);
		m_ActionState.Jump = Input::IsKeyPressed(Key::W, this);

		if (IsRunningClient() && m_ActionState != previous)
		{
			// Send requested action to server
			GetGameModeBase()->Client_SendPlayerAction( [&](BufferStreamWriter& stream) {
				stream.WriteRaw(m_ActionState);
			});
		}
	}

	if (IsRunningClient())
		return;

	// Get SpriteAnimation object reference
	SpriteAnimation& animation = GetSpriteAnimation();
	
	// Set player direction (right: 1.0, left: -1.0f)
	m_Direction = m_ActionState.MoveRight ? 1.0f : (m_ActionState.MoveLeft ? -1.0f : m_Direction);
	
	bool move = m_ActionState.MoveLeft || m_ActionState.MoveRight;

	// Set horizontal velocity (acceleration)
	if (!IsOnHighSlope())
	{
		SetLinearVelocityX(!move ? 0.0f : glm::clamp(
			GetLinearVelocity().x + m_PlayerAcceleration * m_Direction * ts,
			-m_PlayerMaxSpeed, m_PlayerMaxSpeed));
	}

	// Set player state to Run when key is pressed and player is not in the air
	if (move && m_State != Jump && m_JumpTimer >= s_LandAnimationCancelTime)
		m_State = Run;

	// Set player state to Idle when stopped running or landing animation finished playing
	else if (m_State == Run || (m_State == Land && animation.FinishedPlaying()))
		m_State = Idle;

	// Start landing animation
	if (m_State == Jump && IsGrounded())
	{
		m_State = m_JumpTimer >= s_LandAnimationDelay ? Land : Idle;
		m_JumpTimer = 0.0f;
	}

	static Timer impulseTimer;

	// Player pressed a jump key
	if (m_ActionState.Jump && IsGrounded() && m_JumpTimer >= s_JumpDelay)
	{
		ApplyLinearImpulse({ 0.0f,  m_JumpForce });
		m_JumpTimer = 0.0f;
		m_State = Jump;
		impulseTimer.Reset();
	}

	// Player is in the air: Set jump or fall animation frame 
	float velocity = GetLinearVelocity().y;
	if (!IsGrounded())
	{
		// Modify vertical velocity to make jump feel less floaty
		if (velocity > 0.0f && velocity < 0.05f)
			ApplyLinearImpulse({ 0.0f, m_GravityModifier});

		// Update jump animation frame
		uint16_t frame = velocity > 0.0f ? (m_JumpTimer < s_JumpFrameSwitchTime ? 0 : 1) : 2;
		animation.SetAnimationFrame(frame);
		m_State = Jump;
	}

	// Set animation direction when sliding on high slope
	if (IsOnHighSlope() && GetLinearVelocity().y < 0.0f)
		m_Direction = GetLinearVelocity().x > 0.0f ? 1.0f : -1.0f;

	// Update animation and timer
	animation.Play(m_State);
	animation.SetMirrorFlip(m_Direction < 0.0f);
	m_JumpTimer += ts;
}

void Player::OnRegisterFields()
{
	RegisterField(ScriptFieldType::Float, "PlayerMaxSpeed", &m_PlayerMaxSpeed);
	RegisterField(ScriptFieldType::Float, "PlayerAcceleration", &m_PlayerAcceleration);
	RegisterField(ScriptFieldType::Float, "JumpForce", &m_JumpForce);
	RegisterField(ScriptFieldType::Float, "GravityModifier", &m_GravityModifier);
	RegisterField(ScriptFieldType::Int, "ClientID", &m_ClientID, false);
}

void Player::OnImGuiRender()
{
#ifdef PT_EDITOR
	if (GetScene()->IsSimulated())
	{
		auto vel = GetLinearVelocity();
		ImGui::Text("Velocity: (%.3f, %.3f)", vel.x, vel.y);
	}
#endif
}
