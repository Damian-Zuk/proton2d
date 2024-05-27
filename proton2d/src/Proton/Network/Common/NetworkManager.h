#pragma once
#include "Proton/Network/Common/Common.h"

namespace proton {

	class GameInstance;
	class SceneManager;
	class Scene;

	class Client;
	class Server;

	class NetworkManager
	{
	public:
		NetworkManager(GameInstance* instance, SceneManager* manager);
		virtual ~NetworkManager();

		void OnUpdate(float ts);

		void OnSceneSimulationStart(Scene* scene);
		void OnSceneSimulationStop(Scene* scene);

		void StartServer();
		void StopServer();

		void StartClient();
		void StopClient();

		void SetNetMode(NetMode mode);
		NetMode GetNetMode() const { return m_NetMode; }
		bool IsNetModeServer() const { return m_NetMode == NetMode::ListenServer || m_NetMode == NetMode::DedicatedServer; }
		bool IsNetModeClient() const { return m_NetMode == NetMode::Client; }
		bool IsNetServiceRunning() const { return m_IsNetworkServiceRunning; }

		void SetServerTickRate(uint16_t tickRate);

		Client* GetClient();
		Server* GetServer();

	private:
		void CheckNetworkResourcesRelease();
		
	private:
		NetMode m_NetMode = NetMode::ListenServer;

		std::string m_IpAddress = "127.0.0.1";
		int m_Port = 8192;

		uint16_t m_ServerTickRate = 16;
		float m_ServerTickTime = 1.0f / m_ServerTickRate;
		float m_ServerTickElapsed = 0.0f;

		bool m_ClientGameStateInitialized = false;

		GameInstance* m_GameInstance;
		SceneManager* m_SceneManager;

		Unique<Client> m_Client;
		Unique<Server> m_Server;

		bool m_IsNetworkServiceRunning = false;
		uint32_t m_NetworkedSceneCount = 0;

		// Network statistics: Only for server
		bool m_SaveNetworkStatsToLogFile = false; 
		bool m_SaveStatsForAllClients = false;
		uint32_t m_SaveStatsForClientID = 0;

		static uint32_t s_NetworkServicesRunning; // across all editor's game instances (server + clients)
		static bool s_NetworkResourcesFreed; // free resources after all network services finished running

		friend class Application;
		friend class Scene;
		friend class Client;
		friend class Server;

		friend class SettingsPanel;
		friend class InspectorPanel;
		friend class InfoPanel;
	};

}
