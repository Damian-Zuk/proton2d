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

	// Forward declarations
	class NetworkManager;
	class GameInstance;
	class NetReplicator;
	class NetTransformSystem;
	class NetStatistics;

	class Server
	{
	public:
		Server(GameInstance* gameInstance);
		Server() = delete;
		virtual ~Server();

		void Start(uint16_t port);
		void Stop();

		bool IsClientConnected(ClientID clientID) const;
		const ClientInfo& GetClientInfo(ClientID clientID) const;

		void SetClientName(ClientID clientID, const char* name);
		void KickClient(ClientID clientID);

		Entity GetClientEntity(ClientID clientID);
		void SetClientEntity(ClientID clientID, Entity entity);

		// ClientID == 0 -> send message to all clients
		void SendCustomMessage(ClientID clientID, const NetworkStreamWriterDelegate& delegate);

		void SetPacketFakeLag(float latencyMs);

		uint32_t GetConnectedClientsCount() const;
		bool IsRunning() const { return m_Running; }

	private:
		// Game server functionality (executed on main thread)
		void OnTick();
		void OnNetworkMessage(ISteamNetworkingMessage* message);

		void ProcessConnectionStatusQueue();
		void ProcessClientMessagesQueue();

		void OnClientConnecting(ClientID clientID);
		void OnClientConnected(ClientID clientID);
		void OnClientDisconnected(ClientID clientID);
		
		void OnEntitySpawned(Scene* scene, UUID entityUUID);
		void OnEntityDespawned(Scene* scene, UUID entityUUID);

		// Network thread and lower-level server functionality
		void NetworkThreadFunction(); 

		static void ConnectionStatusChangedCallback(SteamNetConnectionStatusChangedCallback_t* info);
		void OnConnectionStatusChanged(SteamNetConnectionStatusChangedCallback_t* info);

		void PollIncomingMessages();
		void PollConnectionStateChanges();
		void OnFatalError(const std::string& message);

		void SendBufferToClient(ClientID clientID, Buffer buffer, bool reliable = true);
		void SendBufferToAllClients(Buffer buffer, ClientID excludeClientID = 0, bool reliable = true);

	private:
		// Can only have one server instance per-process
		inline static Server* s_Instance = nullptr;

		GameInstance* m_GameInstance;
		NetworkManager* m_NetworkManager;

		Unique<NetReplicator> m_NetReplicator;
		Unique<NetTransformSystem> m_NetTransformSystem;
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

		// Buffer for writting network messages
		Buffer m_ScratchBuffer;

		// Connections
		std::map<HSteamNetConnection, ClientInfo> m_ConnectedClients;
		std::queue<std::pair<ClientID, ConnectionStatus>> m_ConnectionStatusChangeQueue;
		std::mutex m_ConnectionStatusChangeQueueMutex;

		// Queue for message processing
		std::queue<ISteamNetworkingMessage*> m_MessageQueue;
		std::mutex m_MessageQueueMutex;

		// Client entity mapping
		std::unordered_map<ClientID, Entity> m_ClientToEntityMap;

		// Debug
		inline static float s_FakeServerLag = 50.0f;

		friend class NetworkManager;
		friend class NetReplicator;
		friend class NetStatistics;
		friend class NetTransformSystem;
		friend class GameModeBase;
		friend class Scene;
	
		friend class SettingsPanel;
		friend class InfoPanel;
		friend class ToolbarPanel;
	};

}
