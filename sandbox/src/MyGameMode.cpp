#include <Proton.h>
using namespace proton;

#include "MyGameMode.h"
#include "Scripts/Player.h"

bool MyGameMode::OnCreate()
{
	if (HasAuthority())
	{
		auto& spawn = GetScene()->FindByTag("PlayerSpawn1").GetTransform();
		Entity localPlayer = PrefabManager::SpawnPrefab(GetScene(), "Player.prefab.json");
		localPlayer.SetWorldPosition(spawn.WorldPosition);
	}

	return true;
}

void MyGameMode::OnUpdate(float ts)
{
}

void MyGameMode::OnDestroy()
{
}

void MyGameMode::Server_OnClientConnected(uint32_t clientID)
{
	PT_TRACE("{}", clientID);
	// Spawn player entity...
	auto& spawn = GetScene()->FindByTag("PlayerSpawn2").GetTransform();
	Entity remotePlayer = PrefabManager::SpawnPrefab(GetScene(), "Player.prefab.json");
	remotePlayer.SetWorldPosition(spawn.WorldPosition);
	Player* script = dynamic_cast<Player*>(remotePlayer.GetScriptInstance("Player"));
	script->m_IsLocalPlayer = false;
	script->m_ClientID = clientID;
	m_RemotePlayers[clientID] = remotePlayer;
}

void MyGameMode::Server_OnClientDisconnected(uint32_t clientID)
{
	PT_TRACE("{}", clientID);
	// Destroy player entity...
	m_RemotePlayers.at(clientID).Destroy();
	m_RemotePlayers.erase(clientID);
}
