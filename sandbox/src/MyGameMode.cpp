#include <Proton.h>
using namespace proton;

#include "MyGameMode.h"
#include "Scripts/Player.h"

bool MyGameMode::OnCreate()
{
	if (HasAuthority())
	{
		auto& spawn = GetScene()->FindByTag("PlayerSpawn1").GetTransform();
		m_LocalPlayer = PrefabManager::SpawnPrefab(GetScene(), "Player.prefab.json").CastTo<Player>();
		m_LocalPlayer->SetWorldPosition(spawn.WorldPosition);
	}
	return true;
}

void MyGameMode::Server_OnClientConnected(uint32_t clientID)
{
	PT_TRACE("{}", clientID);
	std::string spawnPoint = "PlayerSpawn" + std::to_string(1 + (m_RemotePlayers.size() + 1) % 5);
	auto& spawn = GetScene()->FindByTag(spawnPoint).GetTransform();
	Entity entity = PrefabManager::SpawnPrefab(GetScene(), "Player.prefab.json");
	Player* player = entity.CastTo<Player>();
	
	player->SetWorldPosition(spawn.WorldPosition);
	player->m_IsLocalPlayer = false;
	player->m_ClientID = clientID;

	Server_OnEntityCreated(*player);

	// Send info about all clients to connected client
	Server_OnEntityCreated(*m_LocalPlayer, clientID);
	for (auto& [playerID, player] : m_RemotePlayers)
		Server_OnEntityCreated(*player, clientID);
	
	m_RemotePlayers[clientID] = player;
}

void MyGameMode::Server_OnClientDisconnected(uint32_t clientID)
{
	PT_TRACE("{}", clientID);
	Entity entity = *m_RemotePlayers.at(clientID);
	Server_OnEntityDestroyed(entity);
	entity.Destroy();
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


