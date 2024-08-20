#include "ptpch.h"
#include "Proton/Network/NetworkManager.h"
#include "Proton/Network/Client.h"
#include "Proton/Network/Server.h"
#include "Proton/Network/NetTransformSystem.h"

#include "Proton/Core/GameInstance.h"
#include "Proton/Scene/Scene.h"

#include <steam/steamnetworkingsockets.h>
#include <steam/isteamnetworkingutils.h>
#ifndef STEAMNETWORKINGSOCKETS_OPENSOURCE
#include <steam/steam_api.h>
#endif

namespace proton {

	uint32_t NetworkManager::s_GameProtocolVersion = 1;
	uint32_t NetworkManager::s_NetworkServicesRunning = 0;
	bool NetworkManager::s_NetworkDriverInitialized = false;

	NetworkManager::NetworkManager(GameInstance* instance)
		: m_GameInstance(instance)
	{
	}

	NetworkManager::~NetworkManager()
	{
		if (!m_IsNetworkActive)
			return;

		if (IsNetModeServer())
			StopServer();
		else
			StopClient();
	}

	void NetworkManager::OnUpdate(float ts)
	{
		if (!m_IsNetworkActive)
			return;

		m_IsNetworkTick = false;
		if (m_TickElapsed <= 0)
		{
			m_IsNetworkTick = true;
			m_TickElapsed = m_TickTime;
		}
		else
			m_TickElapsed -= ts;

		if (IsNetModeServer())
		{
			if (m_IsNetworkTick)
				m_Server->OnTick();
		}
		else
		{
			m_Client->OnUpdate(ts);
		}
	}

	void NetworkManager::OnSceneSimulationStart(Scene* scene)
	{
		if (!scene->m_EnableNetworking || m_NetMode == NetMode::Standalone)
			return;

		m_NetworkedSceneCount++;

		if (m_IsNetworkActive)
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

		if (m_IsNetworkActive && m_NetworkedSceneCount == 0)
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

		m_IsNetworkActive = true;
		s_NetworkServicesRunning++;
		s_NetworkDriverInitialized = true;
	}

	void NetworkManager::StartClient()
	{
		m_Client = MakeUnique<Client>(m_GameInstance);
		m_Client->ConnectToServer(m_IpAddress + ":" + std::to_string(m_Port));

		m_IsNetworkActive = true;
		s_NetworkServicesRunning++;
		s_NetworkDriverInitialized = true;
	}

	void NetworkManager::StopServer()
	{
		m_Server->Stop();

		// Wait for network thread to finish running
		while (!m_Server->m_NetworkThreadFinished)
			std::this_thread::sleep_for(std::chrono::milliseconds(10));

		m_Server.reset();
		s_NetworkServicesRunning--;
		m_IsNetworkActive = false;

		CheckNetworkResourcesRelease();
	}

	void NetworkManager::StopClient()
	{
		m_Client->Disconnect();

		// Wait for network thread to finish running
		while (!m_Client->m_NetworkThreadFinished)
			std::this_thread::sleep_for(std::chrono::milliseconds(10));

		m_Client.reset();
		s_NetworkServicesRunning--;
		m_IsNetworkActive = false;

		CheckNetworkResourcesRelease();
	}

	void NetworkManager::SetIpAddress(const std::string& ip)
	{
		m_IpAddress = ip;
	}

	void NetworkManager::SetNetworkPort(uint16_t port)
	{
		m_Port = port;
	}

	void NetworkManager::SetNetMode(NetMode mode)
	{
		if (m_IsNetworkActive)
		{
			PT_CORE_ERROR("Cannot set NetMode: Network service is already running!");
			return;
		}
		m_NetMode = mode;
	}

	void NetworkManager::SetMaxServerConnections(uint32_t value)
	{
		m_MaxServerConnections = value;
	}

	void NetworkManager::SetTickRate(uint16_t tickRate)
	{
		m_TickRate = tickRate;
		m_TickTime = 1.0f / tickRate;
	}

	ConnectionStatus NetworkManager::GetClientConnectionStatus() const
	{
		if (m_Client)
			return m_Client->m_ConnectionStatus;

		return ConnectionStatus::Disconnected;
	}

	void NetworkManager::SetLocalPlayerEntity(Entity entity)
	{
		m_LocalPlayerEntity = entity;
	}

	void NetworkManager::Client_SetOnCustomMessageCallback(uint16_t messageType, const NetworkStreamReaderDelegate& delegate)
	{
	}

	void NetworkManager::Client_SendCustomMessage(const NetworkStreamWriter& delegate)
	{
	}

	void NetworkManager::Server_SetOnCustomMessageCallback(uint16_t messageType, ClientID clientID, const NetworkStreamReaderDelegate& delegate)
	{
	}

	void NetworkManager::Server_SendCustomMessage(ClientID clientID, const NetworkStreamWriter& delegate)
	{
	}

	void NetworkManager::SetGameProtocolVersion(uint32_t version)
	{
		NetworkManager::s_GameProtocolVersion = version;
	}

	void NetworkManager::CheckNetworkResourcesRelease()
	{
		if (!s_NetworkDriverInitialized && !s_NetworkServicesRunning)
		{
			_PT_CORE_INFO("Network resources have been released");
			GameNetworkingSockets_Kill();
			s_NetworkDriverInitialized = false;
		}
	}

}
