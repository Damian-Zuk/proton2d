#include "ptpch.h"
#include "Proton/Network/Common/NetworkManager.h"
#include "Proton/Network/Client/Client.h"
#include "Proton/Network/Server/Server.h"
#include "Proton/Core/GameInstance.h"
#include "Proton/Scene/Scene.h"

#include <steam/steamnetworkingsockets.h>
#include <steam/isteamnetworkingutils.h>
#ifndef STEAMNETWORKINGSOCKETS_OPENSOURCE
#include <steam/steam_api.h>
#endif

namespace proton {

	uint32_t NetworkManager::s_NetworkServicesRunning = 0;
	bool NetworkManager::s_NetworkResourcesFreed = false;

	NetworkManager::NetworkManager(GameInstance* instance, SceneManager* manager)
		: m_GameInstance(instance), m_SceneManager(manager)
	{
	}

	NetworkManager::~NetworkManager()
	{
		if (m_IsNetworkServiceRunning)
		{
			if (m_NetMode == NetMode::ListenServer || m_NetMode == NetMode::DedicatedServer)
			{
				StopServer();
			}
			else if (m_NetMode == NetMode::Client)
			{
				StopClient();
			}
		}
	}

	void NetworkManager::OnSceneSimulationStart(Scene* scene)
	{
		if (!scene->m_InheritNetMode && m_NetMode != NetMode::Standalone)
			return;

		m_NetworkedSceneCount++;

		if (!m_IsNetworkServiceRunning)
		{
			if (m_NetMode == NetMode::ListenServer || m_NetMode == NetMode::DedicatedServer)
			{
				StartServer();
			}
			else if (m_NetMode == NetMode::Client)
			{
				StartClient();
			}
		}
	}

	void NetworkManager::OnSceneSimulationStop(Scene* scene)
	{
		if (!scene->m_InheritNetMode && m_NetMode != NetMode::Standalone)
			return;

		m_NetworkedSceneCount--;

		if (m_IsNetworkServiceRunning && m_NetworkedSceneCount == 0)
		{
			if (m_NetMode == NetMode::ListenServer || m_NetMode == NetMode::DedicatedServer)
			{
				StopServer();
			}
			else if (m_NetMode == NetMode::Client)
			{
				StopClient();
			}
		}
	}

	void NetworkManager::StartServer()
	{
		m_Server = MakeUnique<Server>(m_GameInstance);
		m_Server->Start(m_Port);

		m_IsNetworkServiceRunning = true;
		s_NetworkServicesRunning++;
		s_NetworkResourcesFreed = false;
	}

	void NetworkManager::StartClient()
	{
		m_Client = MakeUnique<Client>(m_GameInstance);
		m_Client->ConnectToServer(m_IpAddress + ":" + std::to_string(m_Port));

		m_IsNetworkServiceRunning = true;
		s_NetworkServicesRunning++;
		s_NetworkResourcesFreed = false;
	}

	void NetworkManager::StopServer()
	{
		m_Server->Stop();

		// Wait for network thread to finish
		while (!m_Server->m_NetworkThreadFinished)
			std::this_thread::sleep_for(std::chrono::milliseconds(10));

		m_Server.reset();
		s_NetworkServicesRunning--;
		m_IsNetworkServiceRunning = false;

		CheckNetworkResourcesRelease();
	}

	void NetworkManager::StopClient()
	{
		m_Client->Disconnect();

		// Wait for network thread to finish
		while (!m_Client->m_NetworkThreadFinished)
			std::this_thread::sleep_for(std::chrono::milliseconds(10));

		m_Client.reset();
		s_NetworkServicesRunning--;
		m_IsNetworkServiceRunning = false;

		CheckNetworkResourcesRelease();
	}

	void NetworkManager::SetNetMode(NetMode mode)
	{
		if (m_IsNetworkServiceRunning)
		{
			PT_CORE_ERROR("Cannot change NetMode: Network service is already running!");
			return;
		}
		m_NetMode = mode;
	}

	void NetworkManager::CheckNetworkResourcesRelease()
	{
		if (!s_NetworkResourcesFreed && !s_NetworkServicesRunning)
		{
			PT_CORE_INFO("Network resources have been released");
			GameNetworkingSockets_Kill();
			s_NetworkResourcesFreed = true;
		}
	}

}
