#include <Proton.h>
using namespace proton;

#include "MyGameMode.h"
#include "Scripts/Player.h"

constexpr glm::vec4 COLOR_RED    { 1.000f, 0.356f, 0.065f, 1.0f };
constexpr glm::vec4 COLOR_GREEN  { 0.526f, 1.000f, 0.065f, 1.0f };
constexpr glm::vec4 COLOR_BLUE   { 0.209f, 0.431f, 0.987f, 1.0f };
constexpr glm::vec4 COLOR_YELLOW { 0.987f, 1.000f, 0.065f, 1.0f };
constexpr glm::vec4 COLOR_ORANGE { 1.000f, 0.679f, 0.294f, 1.0f };
constexpr glm::vec4 COLOR_CYAN   { 0.065f, 0.793f, 1.000f, 1.0f };
constexpr glm::vec4 COLOR_PURPLE { 0.478f, 0.065f, 1.000f, 1.0f };
constexpr glm::vec4 COLOR_PINK   { 1.000f, 0.472f, 0.952f, 1.0f };
constexpr glm::vec4 COLOR_BLACK  { 0.167f, 0.176f, 0.200f, 1.0f };
constexpr glm::vec4 COLOR_WHITE  { 1.000f, 1.000f, 1.000f, 1.0f };

static const glm::vec4 PlayerColors[] = {
	COLOR_RED, COLOR_GREEN, COLOR_ORANGE, COLOR_YELLOW, COLOR_CYAN, 
	COLOR_PURPLE, COLOR_PINK, COLOR_BLUE, COLOR_WHITE, COLOR_BLACK,
};

bool MyGameMode::OnCreate()
{
	if (HasAuthority())
	{
		// Check if player prefab is already on the scene
		if (Entity player = FindByTag("Player"))
		{
			m_LocalPlayer = player.As<Player>();
			return true;
		}
		
		// Spawn local player
		auto& spawnTransform = FindByTag("PlayerSpawn0").GetTransform();
		m_LocalPlayer = SpawnPrefab("Player").As<Player>();
		m_LocalPlayer->SetWorldPosition(spawnTransform.WorldPosition);
		m_LocalPlayer->SetPlayerColor(COLOR_RED);
	}
	return true;
}

void MyGameMode::OnUpdate(float ts)
{
	if (Entity sampleText = FindByTag("SampleText"))
	{
		if (sampleText.HasComponent<TextComponent>())
		{
			auto& text = sampleText.GetComponent<TextComponent>();
			auto& transform = sampleText.GetTransform();
			//PT_CORE_TRACE("{}", text.FontAsset->CalculateTextSize(&text, transform.Scale.x));
		}
	}

}

void MyGameMode::OnEvent(Event& event)
{
	EventDispatcher(event).Dispatch<KeyPressedEvent>([&](auto& keyEvent)
	{
		if (!HasAuthority())
			return false;

		Scene* scene = GetScene();
		const auto& cursor = scene->GetCursorWorldPosition();

		if (keyEvent.GetKeyCode() == Key::E)
		{
			Entity balls = scene->FindByTag("Balls");
			if (!balls)
				balls = scene->CreateEntity("Balls");
			
			Entity ball = scene->SpawnPrefab("Ball");
			ball.SetWorldPosition(cursor);
			balls.AddChildEntity(ball);
		}
		else if (keyEvent.GetKeyCode() == Key::R)
		{
			Entity boxes = scene->FindByTag("Boxes");
			if (!boxes)
				boxes = scene->CreateEntity("Boxes");
			
			Entity box = scene->SpawnPrefab("Wooden Box");
			box.SetWorldPosition(cursor);
			boxes.AddChildEntity(box);
		}
		return false;
	});
}

void MyGameMode::Server_OnClientConnected(ClientID clientID)
{
	PT_TRACE("client_id={}", clientID);
		
	// Spawn Player object for connected client
	Player* player = SpawnPrefab("Player").As<Player>();
	player->m_ClientID = clientID;
	player->m_IsLocalPlayer = false;

	// Get spawn point position
	uint32_t spawnPoint = (m_RemotePlayers.size() + 1) % 4;
	auto& spawnTransform = FindByTag("PlayerSpawn" + std::to_string(spawnPoint)).GetTransform();
	player->SetWorldPosition(spawnTransform.WorldPosition);

	// Set player color
	player->SetPlayerColor(PlayerColors[m_NewPlayerColorIndex]);
	m_NewPlayerColorIndex = (m_NewPlayerColorIndex + 1) % 10;
	
	m_RemotePlayers[clientID] = player;

	Server_SetClientEntity(clientID, *player);
}

void MyGameMode::Server_OnClientDisconnected(ClientID clientID)
{
	PT_TRACE("client_id={}", clientID);
	
	Player* player = m_RemotePlayers.at(clientID);
	player->Destroy();
	m_RemotePlayers.erase(clientID);
}

void MyGameMode::Client_OnCustomMessage(NetworkStreamReader& stream)
{
	GameMessageType msgType;
	stream.ReadRaw(msgType);

	switch (msgType)
	{
	case GameMessageType::PlayerNick:
	{
		break;
	}
	}
}

void MyGameMode::Server_OnCustomMessage(ClientID clientID, NetworkStreamReader& stream)
{
	GameMessageType msgType;
	stream.ReadRaw(msgType);

	switch (msgType)
	{
	case GameMessageType::PlayerInput:
	{
		Player* remotePlayer = Server_GetClientEntity(clientID).As<Player>();
		if (!stream.ReadRaw(remotePlayer->m_InputState))
			PT_ERROR("Failed to read player input! (client_id={})", clientID);
		break;
	}
	}
}
