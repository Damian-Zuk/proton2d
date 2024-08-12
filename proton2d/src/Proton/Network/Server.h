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
		std::string ConnectionDesc;
		ConnectionStatus Status;
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

		const std::map<HSteamNetConnection, ClientInfo>& GetConnectedClients() const { return m_ConnectedClients; }

	private:
		void Start(uint16_t port);
		void Stop();

		// Game server functionality
		void MainThread_OnTick();

		void SetClientActionCallback(uint32_t clientID, StreamReaderDelegate function);

		void OnClientConnected(const ClientInfo& clientInfo); // called from network thread
		void OnClientDisconnected(const ClientInfo& clientInfo); // called from network thread
		void OnDataReceived(ISteamNetworkingMessage* incomingMessage); // called from network thread

		void ProcessConnectionStatusQueue();
		void ProcessClientMessagesQueue();

		void AddSpawnedEntity(Scene* scene, UUID entityUUID);
		void AddDespawnedEntity(Scene* scene, UUID entityUUID);

		uint32_t GetConnectedClientsCount() const;
		void SetClientNick(HSteamNetConnection hConn, const char* nick);
		void KickClient(ClientID clientID);

		void SetPacketFakeLag(float latencyMs);

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

		// Connections
		std::map<HSteamNetConnection, ClientInfo> m_ConnectedClients;

		// Buffer for writting messages using BufferStreamWriter
		Buffer m_ScratchBuffer;

		// Queue for processing connected or disconnected clients
		struct ClientConnectionStatusChangeInfo
		{
			ClientInfo ClientInfo;
			ConnectionStatus Status;
		};
		std::queue<ClientConnectionStatusChangeInfo> m_ClientConnStatusChangeQueue;
		std::mutex m_ClientConnStatusQueueMutex;

		// Queue for messages processing
		std::queue<ISteamNetworkingMessage*> m_MessageQueue;
		std::mutex m_MessageQueueMutex;

		// Player action callbacks
		std::unordered_map<uint32_t, StreamReaderDelegate> m_PlayerActionCallbacks;

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
