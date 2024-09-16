#include <Proton.h>
using namespace proton;

#include "Player.h"
#include "MyGameMode.h"

// Internal script parameters
static constexpr float s_JumpFrameSwitchTime = 0.3f;
static constexpr float s_LandAnimationMinDelay = 0.5f;

enum SensorType : uint32_t
{
	Sensor_BottomLeft,
	Sensor_BottomRight,
	Sensor_Bottom,
};

void Player::OnRegisterFields()
{
	REGISTER_FIELD(Float, m_JumpImpulse);
	REGISTER_FIELD(Float, m_FallModifier);
	REGISTER_FIELD(Float, m_PlayerSpeed);
	REGISTER_FIELD(Float, m_PlayerAcceleration);
	REGISTER_FIELD_NO_EDIT(Float4, m_PlayerColor);
	REGISTER_FIELD(String, m_PlayerNick);
	REGISTER_FIELD(Int, m_ClientID);
	
	REPLICATED_FIELD(m_ClientID);
	REPLICATED_FIELD(m_PlayerColor, [&]() { SetPlayerColor(m_PlayerColor); });
	REPLICATED_FIELD(m_PlayerNick, [&]() { SetPlayerNick(m_PlayerNick); });

	REPLICATED_DATA(m_Direction);
	REPLICATED_DATA(m_State);
}

bool Player::OnCreate()
{
	// Networking setup
	NetworkManager* networkManager = GetNetworkManager();
	m_IsLocalPlayer = m_ClientID == networkManager->GetLocalClientID();
	
	if (m_IsLocalPlayer)
	{
		GetGameMode<MyGameMode>()->m_LocalPlayer = this;
		GetScene()->SetPrimaryCameraEntity(*this);
		networkManager->SetLocalPlayerEntity(*this);
	}
	else
	{
		// Set sync method to interpolation if not local player
		auto& net = GetComponent<NetworkComponent>();
		net.NetTransform.Method = NetSyncMethod::Interpolation;
	}

	// Setup sprite animations
	AddComponent<SpriteAnimationComponent>();
	SpriteAnimation& animation = GetSpriteAnimation();

	animation.Add(PlayerState_Idle, 10, AnimationPlayMode::REPEAT);
	animation.Add(PlayerState_Run, 8, AnimationPlayMode::REPEAT);
	animation.Add(PlayerState_Jump, 3, AnimationPlayMode::PAUSED);
	animation.Add(PlayerState_Land, 9, AnimationPlayMode::PLAY_ONCE);
	animation.SetFPS(12);

	// Setup physics sensors (child entities)
	SetPhysicsSensor(Sensor_Bottom, "Sensor_Bottom");
	SetPhysicsSensor(Sensor_BottomLeft, "Sensor_BottomLeft");
	SetPhysicsSensor(Sensor_BottomRight, "Sensor_BottomRight");

	m_Wheel = FindChildByTag("Wheel").GetRuntimeBody();
	m_Body = GetRuntimeBody();

	return true;
}

// TODO: Remove this and apply proper force when moving based on body normal
bool Player::IsOnHighSlope() const
{
	return !CheckSensor(Sensor_Bottom) && (CheckSensor(Sensor_BottomLeft) || CheckSensor(Sensor_BottomRight));
}

void Player::OnUpdate(float ts)
{
	SpriteAnimation& animation = GetSpriteAnimation();

	if (m_State == PlayerState_Jump)
	{
		// Update jump animation frame
		glm::vec2 velocity = GetLinearVelocity();
		uint16_t frame = velocity.y > 0.0f ? (m_LastJumpTimer < s_JumpFrameSwitchTime ? 0 : 1) : 2;
		animation.SetAnimationFrame(frame);
	}

	// Play current animation
	animation.Play(m_State);
	animation.SetMirrorFlip(m_Direction < 0.0f);
}

void Player::OnFixedUpdate(float ts)
{
	bool isGrounded = CheckSensor(Sensor_Bottom);

	// Player input logic
	if (m_IsLocalPlayer)
	{
		m_PreviousInputState = m_InputState;
		m_InputState.MoveRight = IsKeyPressed(Key::D);
		m_InputState.MoveLeft = IsKeyPressed(Key::A);
		m_InputState.Jump = IsKeyPressed(Key::Space) || IsKeyPressed(Key::W);

		// Network (sending player input to the server)
		if (IsNetModeClient() && m_InputState != m_PreviousInputState)
		{
			Client_SendCustomMessage([&](NetworkStreamWriter& stream) {
				m_InputState.Jump &= isGrounded; // make sure player can jump
				stream.WriteRaw(GameMessageType::PlayerInput);
				stream.WriteRaw(m_InputState);
			});
		}
	}

	// Set player direction (right: 1.0, left: -1.0f)
	m_Direction = m_InputState.MoveRight ? 1.0f : (m_InputState.MoveLeft ? -1.0f : m_Direction);
	
	// If network client and does not have prediction, stop here
	if (IsNetModeClient() && !HasNetworkPrediction())
		return;
	
	bool move = m_InputState.MoveLeft || m_InputState.MoveRight;

	// Set wheel fixed rotation when not moving
	if (!move)
	{
		if (!IsOnHighSlope())
			m_Wheel->SetFixedRotation(true);
		
		m_Wheel->SetLinearVelocity({ 0.0f, m_Wheel->GetLinearVelocity().y });
	}
	else
		m_Wheel->SetFixedRotation(false);

	// Get player linear velocity
	glm::vec2 velocity = GetLinearVelocity();

	// Set player horizontal velocity
	if (!IsOnHighSlope())
	{
		// Modify max speed because of linear damping (when in air there is no damping)
		float maxSpeed = m_PlayerSpeed * (isGrounded ? 3.0f / 2.0f : 1.0f);
		float newVelocity = velocity.x + m_PlayerAcceleration * 10.0f * m_Direction * ts;
		SetLinearVelocityX(!move ? 0.0f : glm::clamp(newVelocity, -maxSpeed, maxSpeed));
	}
	else
	{
		// Apply force to speed up sliding
		m_Body->ApplyForceToCenter({ m_Direction * 100.0f, 0.0f }, true);
	}

	// Set player state to Run when key is pressed and player is not jumping
	if (move && m_State != PlayerState_Jump)
		m_State = PlayerState_Run;

	// Set player state to Idle when stopped running or landing animation finished playing
	else if (m_State == PlayerState_Run || (m_State == PlayerState_Land && GetSpriteAnimation().FinishedPlaying()))
		m_State = PlayerState_Idle;

	// Start landing animation
	if (m_State == PlayerState_Jump && isGrounded)
	{
		m_State = m_LastJumpTimer >= s_LandAnimationMinDelay ? PlayerState_Land : PlayerState_Idle;
		if (glm::abs(velocity.x) > 5.0f)
			m_State = PlayerState_Run;
		m_LastJumpTimer = 0.0f;
	}

	// Player want to jump
	if (m_InputState.Jump && isGrounded)
	{
		SetLinearVelocity(0.0f, 0.0f);
		ApplyLinearImpulse({ 0.0f,  m_JumpImpulse });
		m_LastJumpTimer = 0.0f;
		m_State = PlayerState_Jump;
	}

	// Player is in the air
	if (!isGrounded)
	{
		// Modify vertical velocity to make jump feel less floaty
		if (m_LastJumpTimer >= 0.1f && velocity.y < 0.5f)
			m_Body->ApplyForceToCenter({ 0.0f, -m_FallModifier }, true);

		float linearDamping = m_Body->GetLinearDamping();
		if (linearDamping != 0.0f)
		{
			m_OriginalLinearDamping = m_Body->GetLinearDamping();
			m_Body->SetLinearDamping(0.0f);
		}
		m_State = PlayerState_Jump;
	}
	else
	{
		// Restore linear dumping (to prevent bounces after landing)
		m_Body->SetLinearDamping(m_OriginalLinearDamping);
	}

	// Set direction when sliding on high slope
	if (IsOnHighSlope() && velocity.y < 0.0f)
		m_Direction = velocity.x > 0.0f ? 1.0f : -1.0f;
	
	m_LastJumpTimer += ts;
}

void Player::SetPlayerColor(const glm::vec4& color)
{
	auto& spriteColor = GetComponent<SpriteComponent>().Color;
	spriteColor = color;
	m_PlayerColor = color;
}

void Player::SetPlayerNick(const std::string& nick)
{
	if (Entity entity = FindChildByTag("Text_PlayerName"))
	{
		auto& text = entity.GetComponent<TextComponent>();
		text.TextString = nick;
		text.Color = m_PlayerColor;
		m_PlayerNick = nick;
	}
}

void Player::OnImGuiRender()
{
#ifdef PT_EDITOR
	ImGui::Dummy({ 0, 5 });
	static char buffer[128];
#if 0
	if (ImGui::ColorEdit4("Color", glm::value_ptr(m_PlayerColor)))
		SetPlayerColor(m_PlayerColor);
	const auto& color = GetComponent<SpriteComponent>().Color;
	std::string colorStr = fmt::format("{:.3f}, {:.3f}, {:.3f}, {:.3f}", color.r, color.g, color.b, color.a);
	strcpy_s(buffer, colorStr.c_str());
	ImGui::InputText("Color Float Values", buffer, strlen(buffer), ImGuiInputTextFlags_ReadOnly);
#endif
	ImGui::Text("Is Local Player: %s", m_IsLocalPlayer ? "Yes" : "No");
#endif
}
