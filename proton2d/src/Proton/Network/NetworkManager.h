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

		void OnUpdate(float ts);

		void OnSceneSimulationStart(Scene* scene);
		void OnSceneSimulationStop(Scene* scene);

		void StartServer();
		void StopServer();

		void StartClient();
		void StopClient();

		Client* GetClient();
		Server* GetServer();

		void SetServerIpAddress(const std::string& ip);
		void SetServerPort(int port);

		void SetNetMode(NetMode mode);
		NetMode GetNetMode() const { return m_NetMode; }
		bool IsNetModeServer() const { return m_NetMode == NetMode::ListenServer || m_NetMode == NetMode::DedicatedServer; }
		bool IsNetModeClient() const { return m_NetMode == NetMode::Client; }
		bool IsNetServiceRunning() const { return m_IsNetworkServiceRunning; }

		void SetServerTickRate(uint16_t tickRate);

		uint32_t GetLocalClientID() const { m_LocalClientID; }

		static void SetGameProtocolVersion(uint32_t version);

	private:
		void CheckNetworkResourcesRelease();
		
	private:
		GameInstance* m_GameInstance;
		NetMode m_NetMode = NetMode::ListenServer;

		Unique<Client> m_Client;
		Unique<Server> m_Server;

		std::string m_IpAddress = "127.0.0.1";
		int m_Port = 8192;

		uint16_t m_ServerTickRate = 32;
		float m_ServerTickTime = 1.0f / m_ServerTickRate;
		float m_ServerTickElapsed = 0.0f;

		bool m_ClientGameStateInitialized = false;
		uint32_t m_LocalClientID = 0;

		bool m_IsNetworkServiceRunning = false;
		uint32_t m_NetworkedSceneCount = 0;

		// Network statistics logging settings (only for server)
		bool m_SaveNetworkStatsToLogFile = false; 
		bool m_SaveStatsForAllClients = false;
		uint32_t m_SaveStatsForClientID = 0;

		static uint32_t s_NetworkServicesRunning; // across all editor's game instances (server + clients)
		static uint32_t s_EditorClientInstances; // count of running editor client game instances
		static bool s_NetworkResourcesFreed; // free resources after all network services finished running

		static uint32_t s_GameProtocolVersion;

		friend class Application;
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
