#pragma once
#include "Proton/Network/Common/Network.h"

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

		void OnSceneSimulationStart(Scene* scene);
		void OnSceneSimulationStop(Scene* scene);

		void StartServer();
		void StopServer();

		void StartClient();
		void StopClient();

		void SetNetMode(NetMode mode);
		NetMode GetNetMode() const { return m_NetMode; }

	private:
		void CheckNetworkResourcesRelease();

	private:
		NetMode m_NetMode = NetMode::ListenServer;
		uint32_t m_NetworkedSceneCount = 0;

		std::string m_IpAddress = "127.0.0.1";
		int m_Port = 8192;

		bool m_IsNetworkServiceRunning = false;

		GameInstance* m_GameInstance;
		SceneManager* m_SceneManager;

		Unique<Client> m_Client;
		Unique<Server> m_Server;

		static uint32_t s_NetworkServicesRunning; // across all editor's game instances (server + clients)
		static bool s_NetworkResourcesFreed; // free resources after all network services finished running

		friend class Client;
		friend class Server;
	};

}
