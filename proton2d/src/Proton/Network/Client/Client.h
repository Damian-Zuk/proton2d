#pragma once

#include "Proton/Network/Common/Common.h"

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
	class NetClientTransformSyncSystem;

	class Client
	{
	public:
		enum class ConnectionStatus
		{
			Disconnected = 0, Connected, Connecting, FailedToConnect
		};

		Client(GameInstance* gameInstance);
		~Client();
	
	private:
		void ConnectToServer(const std::string& serverAddress);
		void Disconnect();
		void Shutdown();

		void MainThread_OnUpdate(float ts);
		
		// Game client functionality
		void SendVerifyGameState();
		void SendPlayerAction(Client_SendPlayerActionCallback sendFunction);

		void ProcessMessages();
		void UpdateReplicatedEntities(Scene* scene, BufferStreamReader& stream, uint64_t bufferSize, bool updateTransformNow = false);

		void OnDataReceived(ISteamNetworkingMessage* incomingMessage);
		
		// Client lower-level functionality
		void NetworkThreadFunction();
		
		static void ConnectionStatusChangedCallback(SteamNetConnectionStatusChangedCallback_t* info);
		void OnConnectionStatusChanged(SteamNetConnectionStatusChangedCallback_t* info);

		void PollIncomingMessages();
		void PollConnectionStateChanges();

		void OnFatalError(const std::string& message);

		// Sending buffer to server
		void SendBuffer(Buffer buffer, bool reliable = true);
		void SendString(const std::string& string, bool reliable = true);

		template<typename T>
		void SendData(const T& data, bool reliable = true)
		{
			SendBuffer(Buffer(&data, sizeof(T)), reliable);
		}

		// Other
		bool IsRunning() const { return m_Running; }
		ConnectionStatus GetConnectionStatus() const { return m_ConnectionStatus; }
		const std::string& GetConnectionDebugMessage() const { return m_ConnectionDebugMessage; }

	private:
		// GameNetworkingSockets API
		ISteamNetworkingSockets* m_Interface = nullptr;
		HSteamNetConnection m_Connection = 0;
		std::string m_ServerAddress;

		ConnectionStatus m_ConnectionStatus = ConnectionStatus::Disconnected;
		std::string m_ConnectionDebugMessage;

		// Network thread
		std::thread m_NetworkThread;
		std::atomic<bool> m_NetworkThreadFinished = false;
		bool m_Running = false;

		// Buffer for writting messages using BufferStreamWriter
		Buffer m_ScratchBuffer;

		// Queues
		std::queue<ISteamNetworkingMessage*> m_MessageQueue;
		std::mutex m_QueueMutex;
		
		// Connection info
		uint32_t m_ServerClientID = 0;
		bool m_JustConnected = false;

		// Other
		bool m_GameStateInitialized = false;

		GameInstance* m_GameInstance;
		NetworkManager* m_NetworkManager;

		friend class NetworkManager;
		friend class GameModeBase;
	};

}
