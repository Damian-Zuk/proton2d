#include "ptpch.h"
#include "Proton/Network/NetworkManager.h"
#include "Proton/Network/Client.h"
#include "Proton/Network/Server.h"
#include "Proton/Network/NetTransformSystem.h"

#include "Proton/Core/GameInstance.h"
#include "Proton/Scene/Scene.h"
#include "Proton/Scripting/GameModeBase.h"
#include "Proton/Utils/Utils.h"

#include <steam/steamnetworkingsockets.h>
#include <steam/isteamnetworkingutils.h>
#ifndef STEAMNETWORKINGSOCKETS_OPENSOURCE
#include <steam/steam_api.h>
#endif

#include <nlohmann/json.hpp>

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

	void NetworkManager::SaveConfig() const
	{
		nlohmann::json jsonObj;
		std::ofstream configFile("content/network.json");
		jsonObj["client_name"] = m_ClientName;
		jsonObj["net_mode"] = NetModeToString(m_NetMode);
		jsonObj["server_ip"] = m_IpAddress;
		jsonObj["port"] = m_Port;
		jsonObj["tickrate"] = m_Tickrate;
		jsonObj["use_physics_tickrate"] = m_UsePhysicsTickrate;
		jsonObj["max_server_connections"] = m_MaxServerConnections;
		jsonObj["debug_fake_lag"] = Server::s_FakeServerLag;
		configFile << jsonObj.dump(4);
		configFile.close();
	}

	void NetworkManager::ReadConfig()
	{
		nlohmann::json jsonObj = jsonObj.parse(Utils::ReadFile("content/network.json"));

		if (jsonObj.contains("client_name"))
			m_ClientName = jsonObj["client_name"];

		if (jsonObj.contains("net_mode"))
			m_NetMode = StringToNetMode(jsonObj["net_mode"]);

		if (jsonObj.contains("server_ip"))
			m_IpAddress = jsonObj["server_ip"];

		if (jsonObj.contains("port"))
			m_Port = jsonObj["port"];

		if (jsonObj.contains("tickrate"))
			m_Tickrate = jsonObj["tickrate"];

		if (jsonObj.contains("use_physics_tickrate"))
			m_UsePhysicsTickrate = jsonObj["use_physics_tickrate"];

		if (jsonObj.contains("max_server_connections"))
			m_MaxServerConnections = jsonObj["max_server_connections"];

		if (jsonObj.contains("debug_fake_lag"))
			Server::s_FakeServerLag = jsonObj["debug_fake_lag"];
	}

	void NetworkManager::OnUpdate(float ts)
	{
		if (!m_IsNetworkActive)
			return;

		if (m_UsePhysicsTickrate)
		{
			m_IsNetworkTick = m_GameInstance->GetActiveScene()->IsPhysicsTick();
		}
		else
		{
			m_IsNetworkTick = false;
			if (m_TickElapsed <= 0)
			{
				m_IsNetworkTick = true;
				m_TickElapsed = m_TickTime;
			}
			else
				m_TickElapsed -= ts;
		}

		if (IsNetModeServer())
		{
			if (m_IsNetworkTick)
				m_Server->OnTick();
		}
		else
		{
			if (!m_Client->m_NetworkThreadFinished)
				m_Client->OnUpdate(ts);
			else
				StopClient();
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
		if (!m_Client->m_NetworkThreadFinished)
			m_Client->Disconnect();

		// Wait for network thread to finish running
		while (!m_Client->m_NetworkThreadFinished)
			std::this_thread::sleep_for(std::chrono::milliseconds(10));

		Scene* scene = m_GameInstance->GetActiveScene();
		GameModeBase* gameMode = scene->GetGameMode();
		gameMode->Client_OnDisconnected(m_Client->m_ConnectionEndCode);

		m_Client.reset();
		s_NetworkServicesRunning--;
		m_IsNetworkActive = false;

		CheckNetworkResourcesRelease();
	}

	void NetworkManager::SetIpAddress(const std::string& ip)
	{
		// TODO: Validate ip address
		m_IpAddress = ip;
	}

	void NetworkManager::SetNetworkPort(uint16_t port)
	{
		m_Port = (uint16_t)glm::clamp((int)port, 0, 65535);
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

	void NetworkManager::SetLocalClientName(const std::string& name)
	{
		m_ClientName = name.substr(0, 31);
	}

	void NetworkManager::SetUsePhysicsTickrate(bool use)
	{
		m_UsePhysicsTickrate = use;
	}

	void NetworkManager::SetTickRate(uint16_t tickRate)
	{
		m_Tickrate = tickRate;
		m_TickTime = 1.0f / tickRate;
	}

	float NetworkManager::GetTickTime() const
	{
		if (m_UsePhysicsTickrate)
			return m_GameInstance->GetActiveScene()->m_PhysicsTimestep;

		return m_TickTime;
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

	const ClientInfo& NetworkManager::Server_GetClientInfo(ClientID clientID) const
	{
		return m_Server->GetClientInfo(clientID);
	}

	void NetworkManager::Client_SendCustomMessage(const NetworkStreamWriterDelegate& delegate) const
	{
		PT_CORE_ASSERT(m_Client);
		m_Client->SendCustomMessage(delegate);
	}

	void NetworkManager::Server_SendCustomMessage(ClientID clientID, const NetworkStreamWriterDelegate& delegate) const
	{
		PT_CORE_ASSERT(m_Server);
		m_Server->SendCustomMessage(clientID, delegate);
	}

	void NetworkManager::Server_SetClientEntity(ClientID clientID, Entity entity) const
	{
		PT_CORE_ASSERT(m_Server);
		m_Server->SetClientEntity(clientID, entity);
	}

	Entity NetworkManager::Server_GetClientEntity(ClientID clientID) const
	{
		PT_CORE_ASSERT(m_Server);
		return m_Server->GetClientEntity(clientID);
	}

	float NetworkManager::Client_GetCurrentLag() const
	{
		PT_CORE_ASSERT(m_Client);
		return m_Client->GetCurrentLag();
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
