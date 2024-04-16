#include <Proton.h>
using namespace proton;

#include "MyGameMode.h"
#include "Scripts/Player.h"

bool MyGameMode::OnCreate()
{
	if (HasAuthority())
	{
		if (Entity player = FindByTag("NewPlayer"))
		{
			m_LocalPlayer = player.As<Player>();
			return true;
		}
		// Spawn local player for listen server
		auto& spawnTransform = FindByTag("PlayerSpawn0").GetTransform();
		m_LocalPlayer = SpawnPrefab("NewPlayer").As<Player>();
		m_LocalPlayer->SetWorldPosition(spawnTransform.WorldPosition);
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

		return false;
	});
}

void MyGameMode::Server_OnClientConnected(uint32_t clientID)
{
	PT_TRACE("client_id={}", clientID);
		
	// Spawn Player object for connected client
	Player* player = SpawnPrefab("NewPlayer").As<Player>();

	// Get spawn point position
	uint32_t spawnPoint = (m_RemotePlayers.size() + 1) % 4;
	auto& spawnTransform = FindByTag("PlayerSpawn" + std::to_string(spawnPoint)).GetTransform();

	// Set player properties
	player->SetWorldPosition(spawnTransform.WorldPosition);
	player->m_IsLocalPlayer = false;
	player->m_ClientID = clientID;
	
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
}
