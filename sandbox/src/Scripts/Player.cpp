#include <Proton.h>
using namespace proton;

#include "Player.h"
#include "MyGameMode.h"

#include "Proton/Graphics/Renderer/Renderer.h"

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

void Player::OnRegisterFields()
{
	REGISTER_FIELD(Float, m_PlayerMaxSpeed);
	REGISTER_FIELD(Float, m_PlayerAcceleration);
	REGISTER_FIELD(Float, m_JumpForce);
	REGISTER_FIELD(Float, m_FallModifier);
	REGISTER_FIELD(Int, m_ClientID);
	REGISTER_FIELD_NO_EDIT(Float4, m_PlayerColor);
	
	REPLICATED_DATA(m_Direction);
	REPLICATED_DATA(m_State);
	REPLICATED_FIELD(m_ClientID);
	REPLICATED_FIELD(m_PlayerColor, [&]() {
		SetPlayerColor(m_PlayerColor);
	});
}

bool Player::OnCreate()
{
	// Get local player ID
	NetworkManager* networkManager = GetNetworkManager();
	MyGameMode* gameMode = GetGameMode<MyGameMode>();
	uint32_t localPlayerID = gameMode->GetLocalPlayerID();
	m_IsLocalPlayer = m_ClientID == localPlayerID;
	
	if (m_IsLocalPlayer)
	{
		gameMode->m_LocalPlayer = this;
		GetScene()->SetPrimaryCameraEntity(*this);
		
		if (IsRunningClient())
		{
			networkManager->SetLocalPlayerEntity(*this);
			//networkManager->Client_SetEntityInput(GetUUID(), &m_ActionState);
		}
	}
	else
	{
		// Set sync method to default if not local player
		auto& net = GetComponent<NetworkComponent>();
		net.NetTransform.Method = NetSyncMethod::None;
	}

	if (IsRunningServer())
	{
		//GetNetworkManager()->Server_SetCustomMessageCallback(m_ClientID, [])

		GetGameMode()->Server_SetPlayerActionCallback(m_ClientID, [&](NetworkStreamReader& stream) {
			stream.ReadRaw(m_ActionState);
		});
	}

	// Set up sprite animations
	AddComponent<SpriteAnimationComponent>();
	SpriteAnimation& animation = GetSpriteAnimation();

	animation.Add(PlayerState_Idle, 10, AnimationPlayMode::REPEAT);
	animation.Add(PlayerState_Run, 8, AnimationPlayMode::REPEAT);
	animation.Add(PlayerState_Jump, 3, AnimationPlayMode::PAUSED);
	animation.Add(PlayerState_Land, 9, AnimationPlayMode::PLAY_ONCE);
	animation.SetFPS(12);

	// Set physics sensors to following child entities
	SetPhysicsSensor(Sensor_BottomLeft, "Sensor_BottomLeft");
	SetPhysicsSensor(Sensor_BottomRight, "Sensor_BottomRight");
	SetPhysicsSensor(Sensor_Bottom, "Sensor_Bottom");

	m_Wheel = FindChildByTag("Wheel").GetRuntimeBody();
	m_Body = GetRuntimeBody();

	return true;
}

void Player::OnUpdate(float ts)
{
	// Input polling for local player
	if (m_IsLocalPlayer)
	{
		PlayerActionState previous = m_ActionState;
		m_ActionState.MoveRight = IsKeyPressed(Key::D);
		m_ActionState.MoveLeft = IsKeyPressed(Key::A);
		m_ActionState.Jump = IsKeyPressed(Key::W);

		if (IsRunningClient() && m_ActionState != previous)
		{
			GetGameMode()->Client_SendPlayerAction([&](NetworkStreamWriter& stream) {
				stream.WriteRaw(m_ActionState);
			});
		}
	}
	
	SpriteAnimation& animation = GetSpriteAnimation();

	if (m_State == PlayerState_Jump)
	{
		// Update jump animation frame
		glm::vec2 velocity = GetLinearVelocity();
		uint16_t frame = velocity.y > 0.0f ? (m_JumpTimer < s_JumpFrameSwitchTime ? 0 : 1) : 2;
		animation.SetAnimationFrame(frame);
	}

	// Play current animation
	animation.Play(m_State);
	animation.SetMirrorFlip(m_Direction < 0.0f);
}

void Player::OnPhysicsUpdate(float ts)
{
	auto& net = GetComponent<NetworkComponent>();
	if (IsRunningClient() && !m_IsLocalPlayer && net.NetTransform.Method != NetSyncMethod::Prediction)
		return;

	glm::vec2 velocity = GetLinearVelocity();

	// Set player direction (right: 1.0, left: -1.0f)
	m_Direction = m_ActionState.MoveRight ? 1.0f : (m_ActionState.MoveLeft ? -1.0f : m_Direction);
	bool move = m_ActionState.MoveLeft || m_ActionState.MoveRight;

	// Set horizontal velocity (acceleration)
	if (!IsOnHighSlope())
	{
		float maxSpeed = m_PlayerMaxSpeed * (m_State == PlayerState_Run ? 1.0f : 0.8f);
		float newVelocity = velocity.x + m_PlayerAcceleration * m_Direction * ts;

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
		if (glm::abs(velocity.x) > 5.0f)
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
		if (velocity.y < 0.5f && velocity.y > -10.0f)
		{
			m_Body->ApplyForceToCenter({ 0.0f, -m_FallModifier }, true);
		}

		m_State = PlayerState_Jump;
	}

	// Set animation direction when sliding on high slope
	if (IsOnHighSlope() && velocity.y < 0.0f)
		m_Direction = velocity.x > 0.0f ? 1.0f : -1.0f;
	
	m_JumpTimer += ts;
}

bool Player::IsGrounded() const
{
	return CheckSensor(Sensor_Bottom);
}

bool Player::IsOnHighSlope() const
{
	return !CheckSensor(Sensor_Bottom) && (CheckSensor(Sensor_BottomLeft) || CheckSensor(Sensor_BottomRight));
}

void Player::SetPlayerColor(const glm::vec4& color)
{
	auto& spriteColor = GetComponent<SpriteComponent>().Color;
	spriteColor = color;
	m_PlayerColor = color;
}

// --------- Editor ---------
void Player::OnImGuiRender()
{
#ifdef PT_EDITOR
	ImGui::Dummy({ 0, 5 });
	char buffer[128];

	if (ImGui::ColorEdit4("Color", glm::value_ptr(m_PlayerColor)))
		SetPlayerColor(m_PlayerColor);

	const auto& color = GetComponent<SpriteComponent>().Color;
	std::string colorStr = fmt::format("{:.3f}, {:.3f}, {:.3f}, {:.3f}", color.r, color.g, color.b, color.a);
	strcpy_s(buffer, colorStr.c_str());
	ImGui::InputText("Color", buffer, strlen(buffer), ImGuiInputTextFlags_ReadOnly);
	
	if (IsRigidbodyInitialized())
	{
		auto vel = GetLinearVelocity();
		std::string velocity = fmt::format("{:.3f}, {:.3f}", vel.x, vel.y);
		strcpy_s(buffer, velocity.c_str());
		ImGui::InputText("Velocity", buffer, strlen(buffer), ImGuiInputTextFlags_ReadOnly);
		
		ImGui::Text("Gravity scale: %f", m_Body->GetGravityScale());
	}

	ImGui::Text("Is local player: %d", m_IsLocalPlayer);
#endif
}
