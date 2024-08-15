#pragma once
#include "Proton/Network/Common.h"
#include "Proton/Core/UUID.h"

#include <queue>

namespace proton {

	class Scene;
	class Client;
	class Server;
	class NetworkManager;

	using ClientID = uint32_t; // HSteamNetConnection

	class NetReplicator
	{
	public:
		NetReplicator(Client* client);
		NetReplicator(Server* server);

		void Client_ProcessReplicationMessage(NetworkStreamReader& stream);
		void Client_OnEntitySpawnMessage(NetworkStreamReader& stream);
		void Client_OnEntityDespawnMessage(NetworkStreamReader& stream);

		void Server_OnUpdate();
		void Server_OnClientConnected(ClientID clientID);
		void Server_AddSpawnedEntity(Scene* scene, UUID entityUUID);
		void Server_AddDespawnedEntity(Scene* scene, UUID entityUUID);
		void Server_ProcessSpawnedEntityQueue(Scene* scene);
		void Server_ProcessDespawnedEntityQueue(Scene* scene);
		void Server_SendAllSpawnedEntities(Scene* scene, ClientID clientID); // for late joining clients
		void Server_SendAllDespawnedEntities(Scene* scene, ClientID clientID); // for late joining clients

		// If `clientID` is 0, then send update to all clients otherwise to specific client
		// If `forceReplication` is true, then replicate all components despite no changes
		void Server_SendReplicationMessage(ClientID clientID = 0, bool forceReplication = false);

	private:
		Client* m_Client = nullptr;
		Server* m_Server = nullptr;

		// Spawned and despawned entities tracking (server-only)
		struct SceneData
		{
			std::queue<UUID> SpawnedEntityQueue;
			std::queue<UUID> DespawnedEntityQueue;

			// Keep track of all spawned and despawned entities for late joining clients
			std::vector<UUID> SpawnedAll;
			std::vector<UUID> DespawnedAll;
		};
		std::unordered_map<Scene*, SceneData> m_SceneData;
	};

}
