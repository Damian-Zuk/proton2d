#pragma once

#include "Proton/Network/Common/Common.h"
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
		std::string ConnectionDesc;
	};

	// Forward declarations
	class NetworkManager;
	class GameInstance;

	class Server
	{
	public:
		Server(GameInstance* gameInstance);
		~Server();

		struct NetworkStats
		{
			SteamNetConnectionRealTimeStatus_t RealTime;
			Buffer Detailed;
		};

		struct ReplicationStats
		{
			uint32_t RepEntitiesCount = 0;
			uint32_t RepPacketCount = 0;
		};

		bool IsRunning() const { return m_Running; }
		const std::map<HSteamNetConnection, ClientInfo>& GetConnectedClients() const { return m_ConnectedClients; }
		const std::map<HSteamNetConnection, NetworkStats>& GetNetworkStats() const { return m_NetworkStats; }

	private:
		void Start(int port);
		void Stop();

		// Game server functionality
		void MainThread_OnTick();

		void SetClientActionCallback(uint32_t clientID, Server_OnPlayerActionCallback function);
		void PushCreatedEntity(Entity entity, ClientID specificClient = 0);

		void OnClientConnected(const ClientInfo& clientInfo);
		void OnClientDisconnected(const ClientInfo& clientInfo);
		void OnDataReceived(ISteamNetworkingMessage* incomingMessage);

		void SendOnEntityCreated(Entity entity, ClientID specificClient = 0);
		void SendOnEntityDestroyed(Entity entity, ClientID specificClient = 0);

		void ProcessConnectionStatusQueue();
		void ProcessMessages();
		void SendReplicationData();

		// Network statistics
		void InitNetworkStatsForClient(ClientID clientID);
		void ReleaseNetworkStatsForClient(ClientID clientID);
		void UpdateNetworkStatistics();
		void SaveStatsLogsToFile(ClientID clientID, SteamNetConnectionRealTimeStatus_t& status);
		void GenerateStatsLogsFilename();

		void SetClientNick(HSteamNetConnection hConn, const char* nick);
		void KickClient(ClientID clientID);

		// Server lower-level functionality
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
		// GameNetworkingSockets API
		ISteamNetworkingSockets* m_Interface = nullptr;
		HSteamListenSocket m_ListenSocket = 0u;
		HSteamNetPollGroup m_PollGroup = 0u;
		std::map<HSteamNetConnection, ClientInfo> m_ConnectedClients;

		// Network Thread
		std::thread m_NetworkThread;
		std::atomic<bool> m_NetworkThreadFinished = false;
		bool m_Running = false;
		int m_Port = 0;

		// Buffer for writting messages using BufferStreamWriter
		Buffer m_ScratchBuffer;

		// Statistics
		const float m_StatsUpdateInterval = 0.2f;
		std::map<HSteamNetConnection, NetworkStats> m_NetworkStats;
		ReplicationStats m_ReplicationStats;
		std::string m_StatsLogsFilename;
		bool m_StatsLogsHeaderWritten = false;

		// Queues
		struct ClientConnectionStatusChangeInfo
		{
			ClientInfo ClientInfo;
			ConnectionStatus Status;
		};
		std::queue<ClientConnectionStatusChangeInfo> m_ClientConnStatusChangeQueue;
		std::mutex m_ClientConnStatusQueueMutex;

		std::queue<ISteamNetworkingMessage*> m_MessageQueue;
		std::mutex m_MessageQueueMutex;

		struct EntityQueueEntry
		{
			Entity entity;
			ClientID client; // 0 - send to all clients
		};
		std::queue<EntityQueueEntry> m_OnCreatedEntityQueue;

		// Player action callbacks
		std::unordered_map<uint32_t, Server_OnPlayerActionCallback> m_PlayerActionCallbacks;

		// Other
		GameInstance* m_GameInstance;
		NetworkManager* m_NetworkManager;

		friend class NetworkManager;
		friend class GameModeBase;
		friend class Scene;
	};

}
