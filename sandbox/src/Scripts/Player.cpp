#include <Proton.h>
using namespace proton;

#include "Player.h"
#include "MyGameMode.h"

// Internal script parameters
static constexpr float s_JumpDelay = 0.2f;
static constexpr float s_JumpFrameSwitchTime = 0.3f;
static constexpr float s_LandAnimationDelay = 0.5f;
static constexpr float s_LandAnimationCancelTime = 0.2f;

void Player::OnRegisterFields()
{
	RegisterField(ScriptFieldType::Float, "PlayerMaxSpeed", &m_PlayerMaxSpeed);
	RegisterField(ScriptFieldType::Float, "PlayerAcceleration", &m_PlayerAcceleration);
	RegisterField(ScriptFieldType::Float, "JumpForce", &m_JumpForce);
	RegisterField(ScriptFieldType::Float, "GravityModifier", &m_GravityModifier);
	RegisterField(ScriptFieldType::Int, "ClientID", &m_ClientID, false);
}

bool Player::IsTouchingGround() const
{
	return *m_GroundSensorContactCount > 0;
}

bool Player::OnCreate()
{
	m_IsLocalPlayer = m_ClientID == GameModeCastTo<MyGameMode>()->GetLocalPlayerID();

	GetTransform().WorldPosition.z = 0.1f;
	GetTransform().LocalPosition.z = 0.1f;

	if (m_IsLocalPlayer)
		GetScene()->SetPrimaryCameraEntity(*this);

	if (IsRunningClient())
	{
		PT_TRACE("ClientID={}, IsLocalPlayer={}", m_ClientID, m_IsLocalPlayer);
		return true;
	}

	if (IsRunningServer() && !m_IsLocalPlayer)
	{
		GetGameMode()->Server_SetPlayerActionCallback(m_ClientID, [&](BufferStreamReader& stream) {
			stream.ReadRaw(m_ActionState);
		});
	}

	// Set up animations
	AddComponent<SpriteAnimationComponent>();
	SpriteAnimation& animation = GetSpriteAnimation();

	animation.AddAnimation(Idle, 10, AnimationPlayMode::REPEAT);
	animation.AddAnimation(Run,   8, AnimationPlayMode::REPEAT);
	animation.AddAnimation(Jump,  3, AnimationPlayMode::PAUSED);
	animation.AddAnimation(Land,  9, AnimationPlayMode::PLAY_ONCE);
	animation.SetFPS(8);

	// Ground sensor is used to detect if player is touching the ground
	Entity groundDetector = FindChildByTag("GroundDetector");
	Entity groundRightDetector = FindChildByTag("GroundRightDetector");
	Entity groundLeftDetector = FindChildByTag("GroundLeftDetector");
	Entity bottomCollider = FindChildByTag("BottomCollider");

	auto& leftCollider = groundLeftDetector.GetComponent<BoxColliderComponent>();
	m_GroundLeftContactCount = &leftCollider.ContactCallback.ContactCount;

	auto& rightCollider = groundRightDetector.GetComponent<BoxColliderComponent>();
	m_GroundRightContactCount = &rightCollider.ContactCallback.ContactCount;

	auto& circleCollider = bottomCollider.GetComponent<CircleColliderComponent>();
	m_BottomColliderContactCount = &circleCollider.ContactCallback.ContactCount;

	auto& detectorCollider = groundDetector.GetComponent<BoxColliderComponent>();
	m_GroundSensorContactCount = &detectorCollider.ContactCallback.ContactCount;
	
	return true;
}

void Player::OnUpdate(float ts)
{
	if (m_IsLocalPlayer)
	{
		PlayerActionState previous = m_ActionState;
		m_ActionState.MoveRight = Input::IsKeyPressed(Key::D, this);
		m_ActionState.MoveLeft =  Input::IsKeyPressed(Key::A, this);
		m_ActionState.Jump =      Input::IsKeyPressed(Key::W, this);

		if (IsRunningClient() && m_ActionState != previous)
		{
			GetGameMode()->Client_SendPlayerAction([&](BufferStreamWriter& stream) {
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
	SetLinearVelocityX(!move ? 0.0f : glm::clamp(
		GetLinearVelocity().x + m_PlayerAcceleration * m_Direction * ts,
		-m_PlayerMaxSpeed, m_PlayerMaxSpeed));

	// Set player state to Run when key is pressed and player is not in the air
	if (move && m_State != Jump && m_JumpTimer >= s_LandAnimationCancelTime)
		m_State = Run;
	// Set player state to Idle when stopped running or landing animation finished playing
	else if (m_State == Run || (m_State == Land && animation.FinishedPlaying()))
		m_State = Idle;

	// Start landing animation
	if (m_State == Jump && IsTouchingGround())
	{
		m_State = m_JumpTimer >= s_LandAnimationDelay ? Land : Idle;
		m_JumpTimer = 0.0f;
	}

	// Player pressed a jump key
	if (m_ActionState.Jump && IsTouchingGround() && m_JumpTimer >= s_JumpDelay)
	{
		ApplyLinearImpulse({ 0.0f,  m_JumpForce });
		m_JumpTimer = 0.0f;
		m_State = Jump;
	}

	// Player is in the air: Set jump or fall animation frame 
	float velocity = GetLinearVelocity().y;
	if (!IsTouchingGround())
	{
		if (velocity >= -1.0f && m_JumpTimer >= s_LandAnimationCancelTime * 2.0f)
		{
			if (*m_GroundLeftContactCount > 0)
			{
				PT_TRACE("Impulse Left");
				ApplyLinearImpulse({ 10.0f, -10.0f });
			}

			if (*m_GroundRightContactCount > 0)
			{
				PT_TRACE("Impulse Right");
				ApplyLinearImpulse({ -10.0f, -10.0f });
			}
		}

		// Modify vertical velocity to make jump feel less floaty
		if (velocity > 0.0f && velocity < 0.05f)
			ApplyLinearImpulse({ 0.0f, m_GravityModifier});

		// Update jump animation frame
		uint16_t frame = velocity > 0.0f ? (m_JumpTimer < s_JumpFrameSwitchTime ? 0 : 1) : 2;
		animation.SetAnimationFrame(frame);
		m_State = Jump;
	}

	// Update animation and timer
	animation.PlayAnimation(m_State);
	animation.SetMirrorFlip(m_Direction < 0.0f);
	m_JumpTimer += ts;
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
