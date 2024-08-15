#pragma once

#include "Proton/Network/Common.h"
#include "Proton/Scene/Entity.h"

#include <steam/steamnetworkingsockets.h>
#include <steam/isteamnetworkingutils.h>
#ifndef STEAMNETWORKINGSOCKETS_OPENSOURCE
#include <steam/steam_api.h>
#endif

#include <thread>
#include <queue>

namespace proton {

	using ClientID = HSteamNetConnection;

	struct ClientInfo
	{
		ClientID ID;
		ConnectionStatus Status;
		std::string ClientName;
		std::string ConnectionDesc;
	};

	// Forward declarations
	class NetworkManager;
	class GameInstance;
	class NetReplicator;
	class NetStatistics;

	class Server
	{
	public:
		Server(GameInstance* gameInstance);
		~Server();

		bool IsRunning() const { return m_Running; }

		void SetClientEntity(ClientID clientID, Entity entity);
		Entity GetClientEntity(ClientID clientID);

		const std::map<HSteamNetConnection, ClientInfo>& GetConnectedClients() const { return m_ConnectedClients; }

	private:
		void Start(uint16_t port);
		void Stop();

		// Game server functionality
		void MainThread_OnTick();
		void OnNetworkMessage(ISteamNetworkingMessage* message);

		void ProcessConnectionStatusQueue();
		void ProcessClientMessagesQueue();

		void OnClientConnecting(ClientID clientID);
		void OnClientConnected(ClientID clientID);
		void OnClientDisconnected(ClientID clientID);

		uint32_t GetConnectedClientsCount() const;
		void SetClientName(ClientID clientID, const char* name);
		void SetClientActionCallback(uint32_t clientID, NetworkReaderDelegate function);
		void KickClient(ClientID clientID);

		void OnEntitySpawned(Scene* scene, UUID entityUUID);
		void OnEntityDespawned(Scene* scene, UUID entityUUID);

		void SetPacketFakeLag(float latencyMs);

		// Network thread and lower-level server functionality
		void NetworkThreadFunction(); 

		static void ConnectionStatusChangedCallback(SteamNetConnectionStatusChangedCallback_t* info);
		void OnConnectionStatusChanged(SteamNetConnectionStatusChangedCallback_t* info);

		void PollIncomingMessages();
		void PollConnectionStateChanges();
		void OnFatalError(const std::string& message);

		// Sending buffer to clients
		void SendBufferToClient(ClientID clientID, Buffer buffer, bool reliable = true);
		void SendBufferToAllClients(Buffer buffer, ClientID excludeClientID = 0, bool reliable = true);

		void SendStringToClient(ClientID clientID, const std::string& string, bool reliable = true);
		void SendStringToAllClients(const std::string& string, ClientID excludeClientID = 0, bool reliable = true);

		template<typename T>
		void SendDataToClient(ClientID clientID, const T& data, bool reliable = true)
		{
			SendBufferToClient(clientID, Buffer(&data, sizeof(T)), reliable);
		}

		template<typename T>
		void SendDataToAllClients(const T& data, ClientID excludeClientID = 0, bool reliable = true)
		{
			SendBufferToAllClients(Buffer(&data, sizeof(T)), excludeClientID, reliable);
		}

	private:
		static Server* s_Instance;

		GameInstance* m_GameInstance;
		NetworkManager* m_NetworkManager;

		Unique<NetReplicator> m_NetReplicator;
		Unique<NetStatistics> m_NetStatistics;

		// GameNetworkingSockets API
		ISteamNetworkingSockets* m_Interface = nullptr;
		HSteamListenSocket m_ListenSocket = 0u;
		HSteamNetPollGroup m_PollGroup = 0u;

		// Network Thread
		std::thread m_NetworkThread;
		std::atomic<bool> m_NetworkThreadFinished = false;
		bool m_Running = false;
		uint16_t m_Port;

		// Buffer for writting messages using BufferStreamWriter
		Buffer m_ScratchBuffer;

		// Connections
		std::map<HSteamNetConnection, ClientInfo> m_ConnectedClients;
		std::queue<std::pair<ClientID, ConnectionStatus>> m_ConnectionStatusChangeQueue;
		std::mutex m_ConnectionStatusChangeQueueMutex;

		// Queue for messages processing
		std::queue<ISteamNetworkingMessage*> m_MessageQueue;
		std::mutex m_MessageQueueMutex;

		std::unordered_map<ClientID, Entity> m_ClientToEntityMap;

		// Player action callbacks
		std::unordered_map<uint32_t, NetworkReaderDelegate> m_PlayerActionCallbacks;

		// Debug
		static float s_FakeServerLag;

		friend class NetworkManager;
		friend class NetReplicator;
		friend class NetStatistics;
		friend class NetTransformSystem; // TODO: remove
		friend class GameModeBase;
		friend class Scene;
	
		friend class SettingsPanel;
		friend class InfoPanel;
	};

}
