#pragma once
#include "Proton/Network/Common.h"

namespace proton {

	class GameInstance;
	class SceneManager;
	class Scene;

	class Client;
	class Server;

	class NetworkManager
	{
	public:
		NetworkManager(GameInstance* instance);
		virtual ~NetworkManager();

		void StartServer();
		void StopServer();

		void StartClient();
		void StopClient();

		void SetNetMode(NetMode mode);
		void SetNetworkPort(uint16_t port);
		void SetIpAddress(const std::string& ip);
		void SetMaxServerConnections(uint32_t value);
		void SetServerTickRate(uint16_t tickRate);

		NetMode GetNetMode() const { return m_NetMode; }
		uint16_t GetPort() const { return m_Port; }
		bool IsNetModeServer() const { return m_NetMode == NetMode::ListenServer || m_NetMode == NetMode::DedicatedServer; }
		bool IsNetModeClient() const { return m_NetMode == NetMode::Client; }
		bool IsNetworkActive() const { return m_IsNetworkActive; }

		ConnectionStatus GetClientConnectionStatus() const;
		uint32_t GetLocalClientID() const { return m_LocalClientID; }

		Client* GetClient() const { return m_Client.get(); };
		Server* GetServer() const { return m_Server.get(); };

		// Call this function in Application::OnCreate function to set game protocol version
		static void SetGameProtocolVersion(uint32_t version);

	private:
		void OnUpdate(float ts);
		void OnSceneSimulationStart(Scene* scene);
		void OnSceneSimulationStop(Scene* scene);
		void CheckNetworkResourcesRelease();
		
	private:
		GameInstance* m_GameInstance;

		Unique<Client> m_Client;
		Unique<Server> m_Server;

		// General
		NetMode m_NetMode = NetMode::ListenServer;
		std::string m_IpAddress = "127.0.0.1";
		uint16_t m_Port = 8192;

		// Server properties
		uint32_t m_MaxServerConnections = 100;
		uint16_t m_ServerTickRate = 32;

		// Tickrate timer
		float m_ServerTickTime = 1.0f / m_ServerTickRate;
		float m_ServerTickElapsed = 0.0f;

		// True if client has received any replication update from the server
		bool m_ClientGameStateInitialized = false;

		// The local client ID set after receiving HandshakeReply from the server
		uint32_t m_LocalClientID = 0;

		// True if server is running or client is connected/connecting to the server
		bool m_IsNetworkActive = false;
		uint32_t m_NetworkedSceneCount = 0;

		// Network statistics logging options (only for server)
		bool m_SaveNetworkStatsToLogFile = false; 
		bool m_SaveStatsForAllClients = false;
		uint32_t m_SaveStatsForClientID = 0;

		static uint32_t s_GameProtocolVersion; // default value is 1
		static uint32_t s_NetworkServicesRunning; // across all editor game instances (server + clients)
		static bool s_NetworkDriverInitialized; // release GNS resources after all network services stopped

		friend class Application;
		friend class GameInstance;
		friend class Scene;
		friend class Client;
		friend class Server;
		friend class NetStatsManager;
		friend class NetSyncSystem;

		friend class SettingsPanel;
		friend class InspectorPanel;
		friend class InfoPanel;
	};

}
