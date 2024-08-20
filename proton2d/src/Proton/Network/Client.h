#pragma once

#include "Proton/Network/Common.h"

#include <steam/steamnetworkingsockets.h>
#include <steam/isteamnetworkingutils.h>
#ifndef STEAMNETWORKINGSOCKETS_OPENSOURCE
#include <steam/steam_api.h>
#endif

#include <queue>
#include <thread>

namespace proton {

	// Forward declarations
	class GameInstance;
	class Scene;
	class NetworkManager;
	class NetReplicator;
	class NetTransformSystem;

	class Client
	{
	public:
		Client(GameInstance* gameInstance);
		Client() = delete;
		virtual ~Client();

		void ConnectToServer(const std::string& serverAddress);
		void Disconnect();
		void Shutdown();
	
		bool IsRunning() const { return m_Running; }
		ConnectionStatus GetConnectionStatus() const { return m_ConnectionStatus; }
		const std::string& GetConnectionDebugMessage() const { return m_ConnectionDebugMessage; }

	private:
		// Game client functionality (main thread)
		void OnUpdate(float ts);
		void SendHandshake();
		void OnNetworkMessage(ISteamNetworkingMessage* message);
		void SendPlayerAction(NetworkStreamWriterDelegate sendFunction);
		
		// Network thread and client lower-level functionality
		void NetworkThreadFunction();
		
		static void ConnectionStatusChangedCallback(SteamNetConnectionStatusChangedCallback_t* info);
		void OnConnectionStatusChanged(SteamNetConnectionStatusChangedCallback_t* info);

		void PollIncomingMessages();
		void PollConnectionStateChanges();

		void OnFatalError(const std::string& message);

		void SendBuffer(Buffer buffer, bool reliable = true);

	private:
		GameInstance* m_GameInstance;
		NetworkManager* m_NetworkManager;

		Unique<NetReplicator> m_NetReplicator;
		Unique<NetTransformSystem> m_NetTransformSystem;

		// GameNetworkingSockets API
		ISteamNetworkingSockets* m_Interface = nullptr;
		HSteamNetConnection m_Connection = 0;
		std::string m_ServerAddress;

		// Connection info
		ConnectionStatus m_ConnectionStatus = ConnectionStatus::Disconnected;
		std::string m_ConnectionDebugMessage;
		uint32_t m_LocalClientID = 0;

		// Network thread
		std::thread m_NetworkThread;
		std::atomic<bool> m_NetworkThreadFinished = false;
		bool m_Running = false;

		// Preallocated buffer for writting network messages
		Buffer m_ScratchBuffer;

		// Message queue
		std::queue<ISteamNetworkingMessage*> m_MessageQueue;
		std::mutex m_QueueMutex;

		friend class NetworkManager;
		friend class NetReplicator;
		friend class NetTransformSystem;
		friend class GameModeBase;
	};

}
