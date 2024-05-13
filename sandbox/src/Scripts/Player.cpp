#include <Proton.h>
using namespace proton;

#include "Player.h"
#include "MyGameMode.h"

#include <time.h>

// Internal script parameters
static constexpr float s_JumpDelay = 0.1f;
static constexpr float s_JumpFrameSwitchTime = 0.3f;
static constexpr float s_LandAnimationDelay = 0.5f;

enum SensorType : uint32_t
{
	Sensor_BottomLeft,
	Sensor_BottomRight,
	Sensor_Bottom,
};

bool Player::IsGrounded() const
{
	return CheckSensor(Sensor_Bottom);
}

void Player::OnRegisterFields()
{
	PT_REGISTER_FIELD(m_PlayerMaxSpeed, Float);
	PT_REGISTER_FIELD(m_PlayerAcceleration, Float);
	PT_REGISTER_FIELD(m_JumpForce, Float);
	PT_REGISTER_FIELD(m_GravityModifier, Float);

	PT_REGISTER_FIELD(m_ClientID, Int, /*ShowInEditor*/ false, /*NetworkSerialize*/ true);

	PT_REPLICATE_DATA(m_State);
	PT_REPLICATE_DATA(m_Velocity);
}

bool Player::OnCreate()
{
	// Set up sprite animations
	AddComponent<SpriteAnimationComponent>();
	SpriteAnimation& animation = GetSpriteAnimation();

	animation.Add(PlayerState_Idle, 10, AnimationPlayMode::REPEAT);
	animation.Add(PlayerState_Run,   8, AnimationPlayMode::REPEAT);
	animation.Add(PlayerState_Jump,  3, AnimationPlayMode::PAUSED);
	animation.Add(PlayerState_Land,  9, AnimationPlayMode::PLAY_ONCE);
	animation.SetFPS(12);

	uint32_t localPlayerID = GetGameMode<MyGameMode>()->GetLocalPlayerID();
	m_IsLocalPlayer = m_ClientID == localPlayerID;

	if (m_IsLocalPlayer)
	{
		GetScene()->SetPrimaryCameraEntity(*this);
	}

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

	// Set physics sensors to following child entities
	SetPhysicsSensor(Sensor_BottomLeft, "Sensor_BottomLeft");
	SetPhysicsSensor(Sensor_BottomRight, "Sensor_BottomRight");
	SetPhysicsSensor(Sensor_Bottom, "Sensor_Bottom");

	m_Wheel = FindChildByTag("Wheel").GetRuntimeBody();

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
			GetGameModeBase()->Client_SendPlayerAction([&](BufferStreamWriter& stream) {
				stream.WriteRaw(m_ActionState);
			});
		}
	}
	
	if (IsRunningClient())
		m_Direction = m_Velocity.x > 0.0f ? 1.0f : m_Velocity.x < 0.0f ? -1.0f : m_Direction;

	SpriteAnimation& animation = GetSpriteAnimation();

	if (m_State == PlayerState_Jump)
	{
		// Update jump animation frame
		uint16_t frame = m_Velocity.y > 0.0f ? (m_JumpTimer < s_JumpFrameSwitchTime ? 0 : 1) : 2;
		animation.SetAnimationFrame(frame);
	}

	animation.Play(m_State);
	animation.SetMirrorFlip(m_Direction < 0.0f);
}

void Player::OnPhysicsUpdate(float ts)
{
	if (IsRunningClient())
		return;

	m_Velocity = GetLinearVelocity();

	// Set player direction (right: 1.0, left: -1.0f)
	m_Direction = m_ActionState.MoveRight ? 1.0f : (m_ActionState.MoveLeft ? -1.0f : m_Direction);
	bool move = m_ActionState.MoveLeft || m_ActionState.MoveRight;

	// Set horizontal velocity (acceleration)
	if (!IsOnHighSlope())
	{
		float maxSpeed = m_PlayerMaxSpeed * (m_State == PlayerState_Run ? 1.0f : 0.8f);
		float newVelocity = m_Velocity.x + m_PlayerAcceleration * m_Direction * ts;

		SetLinearVelocityX(!move ? 0.0f : glm::clamp(newVelocity, -maxSpeed, maxSpeed));

		if (!move)
		{
			m_Wheel->SetFixedRotation(true);
			m_Wheel->SetLinearVelocity({ 0.0f, m_Wheel->GetLinearVelocity().y });
		}
		else
			m_Wheel->SetFixedRotation(false);
	}

	// Set player state to Run when key is pressed and player is not in the air
	if (move && m_State != PlayerState_Jump)
		m_State = PlayerState_Run;

	// Set player state to Idle when stopped running or landing animation finished playing
	else if (m_State == PlayerState_Run || (m_State == PlayerState_Land && GetSpriteAnimation().FinishedPlaying()))
		m_State = PlayerState_Idle;

	// Start landing animation
	if (m_State == PlayerState_Jump && IsGrounded())
	{
		m_State = m_JumpTimer >= s_LandAnimationDelay ? PlayerState_Land : PlayerState_Idle;
		if (glm::abs(m_Velocity.x) > 5.0f)
			m_State = PlayerState_Run;
		m_JumpTimer = 0.0f;
	}

	// Player pressed a jump key
	if (m_ActionState.Jump && IsGrounded() && m_JumpTimer >= s_JumpDelay)
	{
		SetLinearVelocity(0.0f, 0.0f);
		ApplyLinearImpulse({ 0.0f,  m_JumpForce });
		m_JumpTimer = 0.0f;
		m_State = PlayerState_Jump;
	}

	// Player is in the air: Set jump or fall animation frame 
	if (!IsGrounded())
	{
		// Modify vertical velocity to make jump feel less floaty
		if (m_Velocity.y < 0.5f && m_Velocity.y > -10.0f)
			ApplyLinearImpulse({ 0.0f, m_GravityModifier * 0.1f });

		m_State = PlayerState_Jump;
	}

	// Set animation direction when sliding on high slope
	if (IsOnHighSlope() && m_Velocity.y < 0.0f)
		m_Direction = m_Velocity.x > 0.0f ? 1.0f : -1.0f;
	
	m_JumpTimer += ts;
}

void Player::OnImGuiRender()
{
#ifdef PT_EDITOR
	char buffer[128];
	strcpy_s(buffer, glm::to_string(GetComponent<SpriteComponent>().Color).c_str());
	ImGui::InputText("color", &buffer[0], sizeof(buffer));
	if (GetScene()->IsSimulated())
	{
		auto vel = GetLinearVelocity();
		ImGui::Text("Velocity: (%.3f, %.3f)", vel.x, vel.y);
	}
#endif
}
