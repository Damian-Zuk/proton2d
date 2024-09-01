#pragma once
#include "Proton/Network/Common.h"
#include "Proton/Scene/Entity.h"

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

		void ReadConfig();
		void SaveConfig() const;

		void StartServer();
		void StopServer();

		void StartClient();
		void StopClient();

		void SetNetMode(NetMode mode);
		void SetNetworkPort(uint16_t port);
		void SetIpAddress(const std::string& ip);
		void SetTickRate(uint16_t tickRate);
		void SetMaxServerConnections(uint32_t value);
		void SetLocalClientName(const std::string& name);

		NetMode GetNetMode() const { return m_NetMode; }
		uint16_t GetPort() const { return m_Port; }
		uint16_t GetTickrate() const { return m_TickRate; }
		const std::string& GetLocalClientName() const { return m_ClientName; }
		float GetTickTime() const { return m_TickTime; }
		bool IsNetModeServer() const { return m_NetMode == NetMode::ListenServer || m_NetMode == NetMode::DedicatedServer; }
		bool IsNetModeClient() const { return m_NetMode == NetMode::Client; }
		bool IsNetworkActive() const { return m_IsNetworkActive; }
		bool IsNetworkTick() const { return m_IsNetworkTick; }

		ConnectionStatus GetClientConnectionStatus() const;
		uint32_t GetLocalClientID() const { return m_LocalClientID; }

		void SetLocalPlayerEntity(Entity entity);
		Entity GetLocalPlayerEntity() const { return m_LocalPlayerEntity; }

		void Client_SetOnCustomMessageCallback(uint16_t messageType, const NetworkStreamReaderDelegate& delegate);
		void Client_SendCustomMessage(const NetworkStreamWriter& delegate);

		void Server_SetOnCustomMessageCallback(uint16_t messageType, ClientID clientID, const NetworkStreamReaderDelegate& delegate);
		void Server_SendCustomMessage(ClientID clientID, const NetworkStreamWriter& delegate);

		// Call this function in Application::OnCreate function to set game protocol version
		static void SetGameProtocolVersion(uint32_t version);

	private:
		Client* GetClient() const { return m_Client.get(); };
		Server* GetServer() const { return m_Server.get(); };

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
		uint16_t m_TickRate = 64;

		std::string m_ClientName = "Player";

		// Server properties
		uint32_t m_MaxServerConnections = 100;

		// Tickrate timer
		float m_TickTime = 1.0f / m_TickRate;
		float m_TickElapsed = 0.0f;
		bool m_IsNetworkTick = false;

		// True if server is running or client is connected/connecting to the server
		bool m_IsNetworkActive = false;
		uint32_t m_NetworkedSceneCount = 0;

		// Network statistics logging options (only for server)
		bool m_SaveNetworkStatsToLogFile = false; 
		bool m_SaveStatsForAllClients = false;
		uint32_t m_SaveStatsForClientID = 0;

		// The local client ID set after receiving HandshakeReply from the server
		uint32_t m_LocalClientID = 0;
		Entity m_LocalPlayerEntity;

		static uint32_t s_GameProtocolVersion; // default value is 1
		static uint32_t s_NetworkServicesRunning; // across all editor game instances (server + clients)
		static bool s_NetworkDriverInitialized; // release GNS resources after all network services stopped

		friend class Application;
		friend class GameInstance;
		friend class Scene;
		friend class GameModeBase;

		friend class Client;
		friend class Server;
		friend class NetReplicator;
		friend class NetStatistics;
		friend class NetTransformSystem;

		friend class SettingsPanel;
		friend class InspectorPanel;
		friend class InfoPanel;
	};

}
