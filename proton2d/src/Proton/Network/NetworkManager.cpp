#include "ptpch.h"
#include "Proton/Network/NetworkManager.h"
#include "Proton/Network/Client.h"
#include "Proton/Network/Server.h"
#include "Proton/Network/NetSyncSystem.h"
#include "Proton/Core/GameInstance.h"
#include "Proton/Scene/Scene.h"

#include <steam/steamnetworkingsockets.h>
#include <steam/isteamnetworkingutils.h>
#ifndef STEAMNETWORKINGSOCKETS_OPENSOURCE
#include <steam/steam_api.h>
#endif

#define ENABLE_NET_INTERPOLATION

namespace proton {

	uint32_t NetworkManager::s_NetworkServicesRunning = 0;
	uint32_t NetworkManager::s_EditorClientInstances = 0;
	bool NetworkManager::s_NetworkResourcesFreed = false;

	NetworkManager::NetworkManager(GameInstance* instance)
		: m_GameInstance(instance)
	{
	}

	NetworkManager::~NetworkManager()
	{
		if (!m_IsNetworkServiceRunning)
			return;

		if (IsNetModeServer())
			StopServer();
		else
			StopClient();
	}

	void NetworkManager::OnUpdate(float ts)
	{
		if (!m_IsNetworkServiceRunning)
			return;

		if (IsNetModeServer())
		{
			if (m_ServerTickElapsed <= 0)
			{
				m_Server->MainThread_OnTick();
				m_ServerTickElapsed = m_ServerTickTime;
			}
			else
				m_ServerTickElapsed -= ts;
		}
		else
		{
			m_Client->MainThread_OnUpdate(ts);
		}
	}

	void NetworkManager::OnSceneSimulationStart(Scene* scene)
	{
		if (!scene->m_EnableNetworking || m_NetMode == NetMode::Standalone)
			return;

		m_NetworkedSceneCount++;

		if (m_IsNetworkServiceRunning)
			return;

		if (IsNetModeServer())
			StartServer();
		else
			StartClient();
	}

	void NetworkManager::OnSceneSimulationStop(Scene* scene)
	{
		if (!scene->m_EnableNetworking || m_NetMode == NetMode::Standalone)
			return;

		m_NetworkedSceneCount--;

		if (m_IsNetworkServiceRunning && m_NetworkedSceneCount == 0)
		{
			if (IsNetModeServer())
				StopServer();
			else
				StopClient();
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
		s_EditorClientInstances++;
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
		s_EditorClientInstances--;
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

	void NetworkManager::SetServerIpAddress(const std::string& ip)
	{
		m_IpAddress = ip;
	}

	void NetworkManager::SetServerPort(int port)
	{
		m_Port = port;
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

	void NetworkManager::SetServerTickRate(uint16_t tickRate)
	{
		m_ServerTickRate = tickRate;
		m_ServerTickTime = 1.0f / tickRate;
	}

	Client* NetworkManager::GetClient()
	{
		return m_Client.get();
	}

	Server* NetworkManager::GetServer()
	{
		return m_Server.get();
	}

	void NetworkManager::CheckNetworkResourcesRelease()
	{
		if (!s_NetworkResourcesFreed && !s_NetworkServicesRunning)
		{
			_PT_CORE_INFO("Network resources have been released");
			GameNetworkingSockets_Kill();
			s_NetworkResourcesFreed = true;
		}
	}

}
