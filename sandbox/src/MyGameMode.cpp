#include <Proton.h>
using namespace proton;

#include "MyGameMode.h"
#include "Scripts/Player.h"

#define COLOR_RED    glm::vec4{ 1.0f, 0.356f, 0.065f, 1.0f }
#define COLOR_GREEN  glm::vec4{ 0.526f, 1.0f, 0.065f, 1.0f }
#define COLOR_BLUE   glm::vec4{ 0.209f, 0.431f, 0.987f, 1.0f }
#define COLOR_YELLOW glm::vec4{ 0.987f, 1.0f, 0.065f, 1.0f }
#define COLOR_ORANGE glm::vec4{ 1.0f, 0.679f, 0.294f, 1.0f }
#define COLOR_CYAN   glm::vec4{ 0.065f, 0.793f, 1.0f, 1.0f }
#define COLOR_PURPLE glm::vec4{ 0.478f, 0.065f, 1.0f, 1.0f }
#define COLOR_PINK   glm::vec4{ 1.00f, 0.472f, 0.952f, 1.0f }
#define COLOR_BLACK  glm::vec4{ 0.167f, 0.176f, 0.2f, 1.0f }
#define COLOR_WHITE  glm::vec4{ 1.0f, 1.0f, 1.0f, 1.0f }

static const glm::vec4 s_PlayerColors[10] = {
	COLOR_GREEN, COLOR_YELLOW, COLOR_RED, COLOR_ORANGE, COLOR_CYAN,
	COLOR_BLUE, COLOR_PURPLE, COLOR_PINK, COLOR_BLACK, COLOR_WHITE
};

bool MyGameMode::OnCreate()
{
	if (HasAuthority())
	{
		if (Entity player = FindByTag("Player"))
		{
			m_LocalPlayer = player.As<Player>();
			return true;
		}
		
		// Spawn local player
		auto& spawnTransform = FindByTag("PlayerSpawn0").GetTransform();
		m_LocalPlayer = SpawnPrefab("Player").As<Player>();
		m_LocalPlayer->SetWorldPosition(spawnTransform.WorldPosition);
		m_LocalPlayer->GetColor() = COLOR_GREEN;
	}
	return true;
}

void MyGameMode::OnEvent(Event& event)
{
	EventDispatcher(event).Dispatch<KeyPressedEvent>( [&](auto& keyEvent)
	{
		if (keyEvent.GetKeyCode() == Key::E)
		{
			if (GetScene()->IsSimulated())
				SpawnRandomBox(GetScene()->GetCursorWorldPosition());
		}
		else if(keyEvent.GetKeyCode() == Key::F)
		{
			b2Body* body = m_LocalPlayer->GetRuntimeBody();
			auto& net = m_LocalPlayer->GetComponent<NetworkComponent>();
			auto& t = net.CurrentTransform;
			body->SetTransform({ t.Position.x, t.Position.y }, 0.0f);
		}
		return false;
	});
}

void MyGameMode::Server_OnClientConnected(uint32_t clientID)
{
	PT_TRACE("client_id={}", clientID);
		
	// Spawn Player object for connected client
	Player* player = SpawnPrefab("Player").As<Player>();

	// Get spawn point position
	uint32_t spawnPoint = (m_RemotePlayers.size() + 1) % 4;
	auto& spawnTransform = FindByTag("PlayerSpawn" + std::to_string(spawnPoint)).GetTransform();

	// Set player properties
	player->SetWorldPosition(spawnTransform.WorldPosition);
	player->m_IsLocalPlayer = false;
	player->m_ClientID = clientID;
	player->GetColor() = s_PlayerColors[(m_NewColorIndex % 10)];
	m_NewColorIndex++;
	
	m_RemotePlayers[clientID] = player;
}

void MyGameMode::Server_OnClientDisconnected(uint32_t clientID)
{
	PT_TRACE("client_id={}", clientID);
	
	Player* player = m_RemotePlayers.at(clientID);
	player->Destroy();
	m_RemotePlayers.erase(clientID);
}

void MyGameMode::Client_OnConnected(uint32_t clientID)
{
	m_LocalPlayerID = clientID;
}

uint32_t MyGameMode::GetLocalPlayerID() const
{
	return m_LocalPlayerID;
}

void MyGameMode::SpawnRandomBox(const glm::vec2& position)
{
	Entity entity = GetScene()->CreateEntity("Random Box");

	entity.SetWorldPosition({ position.x, position.y, 0 });
	entity.SetRotationCenter(Random::Float(0.0f, 80.0f));

	auto& sprite = entity.AddComponent<SpriteComponent>("box.png");
	sprite.Color.r = Random::Float(0.0f, 1.0f);
	sprite.Color.g = Random::Float(0.0f, 1.0f);
	sprite.Color.b = Random::Float(0.0f, 1.0f);

	entity.AddComponent<RigidbodyComponent>().Type = b2_dynamicBody;
	entity.AddComponent<BoxColliderComponent>();
	entity.AddComponent<NetworkComponent>();
}
