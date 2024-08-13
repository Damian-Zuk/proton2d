#include "ptpch.h"
#include "Proton/Network/Server.h"
#include "Proton/Network/Messages.h"
#include "Proton/Network/NetworkManager.h"
#include "Proton/Network/NetReplicator.h"
#include "Proton/Network/NetStatistics.h"

#include "Proton/Core/GameInstance.h"
#include "Proton/Core/Timer.h"
#include "Proton/Scene/SceneSerializer.h"
#include "Proton/Scene/Scene.h"
#include "Proton/Scene/SceneManager.h"
#include "Proton/Scene/Entity.h"
#include "Proton/Scripting/GameModeBase.h"
#include "Proton/Scripting/EntityScript.h"

#ifdef PT_EDITOR
#include "Proton/Editor/Panels/InfoPanel.h"
#endif

#include <chrono>
#include <iomanip>
#include <time.h>
#include <spdlog/spdlog.h>

namespace proton {

	// 1 MB scratch buffer for writting messages
	constexpr static size_t s_ScratchBufferSize = 1048576;

	// Can only have one server instance per-process
	Server* Server::s_Instance = nullptr;
	float Server::s_FakeServerLag = 0.0f;

	Server::Server(GameInstance* gameInstance)
		: m_GameInstance(gameInstance), m_NetworkManager(gameInstance->m_NetworkManager.get()),
		m_NetReplicator(MakeUnique<NetReplicator>(this)),
		m_NetStatistics(MakeUnique<NetStatistics>(this))
	{
		m_ScratchBuffer.Allocate(s_ScratchBufferSize);
		SetPacketFakeLag(s_FakeServerLag);
	}

	Server::~Server()
	{
		if (m_NetworkThread.joinable())
			m_NetworkThread.join();

		m_ScratchBuffer.Release();
		Server::s_Instance = nullptr;
	}

	void Server::Start(uint16_t port)
	{
		if (m_Running)
		{
			PT_CORE_WARN("Server is already running!");
			return;
		}

		m_Port = port;
		m_NetworkThread = std::thread([this]() { NetworkThreadFunction(); });
	}

	void Server::Stop()
	{
		m_Running = false;
	}

	void Server::MainThread_OnTick()
	{
		PROFILE_FUNCTION();
		Scene* scene = m_GameInstance->GetActiveScene();

		ProcessConnectionStatusQueue();
		ProcessClientMessagesQueue();
		m_NetReplicator->Server_OnUpdate();	
		m_NetStatistics->UpdateNetworkStatistics();
	}

	void Server::OnNetworkMessage(ISteamNetworkingMessage* message)
	{
		uint32_t clientID = message->m_conn;
		Buffer buffer(message->m_pData, message->m_cbSize);
		BufferStreamReader reader(buffer);
		BufferStreamWriter writer(m_ScratchBuffer);

		MessageType packetType;
		reader.ReadRaw(packetType);
		reader.SetStreamPosition(0);

		switch (packetType)
		{
		////////////////////////////////////////////////////////////////////////////////////////////////////
		case MessageType::Handshake:
		{
			NetMessageHandshake handshake;
			reader.ReadRaw(handshake);

			if (handshake.EngineProtocolVersion != PROTON_NET_PROTOCOL_VERSION && handshake.GameProtocolVersion != NetworkManager::s_GameProtocolVersion)
			{
				PT_CORE_WARN("Handshake failed client_id={} error_code={}", clientID, NetConnectionEndCode_GameAndEngineProtocolMismatch);
				m_Interface->CloseConnection(clientID, NetConnectionEndCode_GameAndEngineProtocolMismatch, "Failed to handshake", false);
				break;
			}
			else if (handshake.GameProtocolVersion != NetworkManager::s_GameProtocolVersion)
			{
				PT_CORE_WARN("Handshake failed client_id={} error_code={}", clientID, NetConnectionEndCode_GameProtocolMismatch);
				m_Interface->CloseConnection(clientID, NetConnectionEndCode_GameProtocolMismatch, "Failed to handshake", false);
				break;
			}
			else if (handshake.EngineProtocolVersion != PROTON_NET_PROTOCOL_VERSION)
			{
				PT_CORE_WARN("Handshake failed client_id={} error_code={}", clientID, NetConnectionEndCode_EngineProtocolMismatch);
				m_Interface->CloseConnection(clientID, NetConnectionEndCode_EngineProtocolMismatch, "Failed to handshake", false);
				break;
			}

			NetMessageHandshakeReply reply;
			reply.ClientID = clientID;
			writer.WriteRaw(reply);
			SendBufferToClient(clientID, writer.GetBuffer());

			auto& clientInfo = m_ConnectedClients[clientID];
			clientInfo.Status = ConnectionStatus::Connected;
			OnClientConnected(clientID);

			break;
		}
		////////////////////////////////////////////////////////////////////////////////////////////////////
		case MessageType::PlayerAction:
		{
			PROFILE_SCOPE("PacketType::PlayerAction");

			reader.SkipBytes(sizeof(NetMassagePlayerAction));
			if (m_PlayerActionCallbacks.find(clientID) != m_PlayerActionCallbacks.end())
			{
				StreamReaderDelegate& callback = m_PlayerActionCallbacks.at(clientID);
				callback(reader);
			}
			else
				PT_CORE_ERROR("PlayerActionCallback was not defined! client_id={}", clientID);

			break;
		}
		////////////////////////////////////////////////////////////////////////////////////////////////////
		default:
			PT_CORE_ERROR("Invalid packet type ({}).", (uint16_t)packetType);
			KickClient(message->GetConnection());
			break;
		}
	}

	void Server::ProcessClientMessagesQueue()
	{
		PROFILE_FUNCTION();
		std::lock_guard<std::mutex> lock(m_MessageQueueMutex);

		while (!m_MessageQueue.empty())
		{
			ISteamNetworkingMessage* message = m_MessageQueue.front();
			OnNetworkMessage(message);
			message->Release();
			m_MessageQueue.pop();
		}
	}

	void Server::ProcessConnectionStatusQueue()
	{
		std::lock_guard<std::mutex> lock(m_ConnectionStatusChangeQueueMutex);

		while (!m_ConnectionStatusChangeQueue.empty())
		{
			const auto& connectionInfo = m_ConnectionStatusChangeQueue.front();
			const auto& clientID = connectionInfo.first;
			const auto& status = connectionInfo.second;

			if (status == ConnectionStatus::Connecting)
			{
				OnClientConnecting(clientID);
			}
			else if (status == ConnectionStatus::Disconnected)
			{
				OnClientDisconnected(clientID);
			}

			m_ConnectionStatusChangeQueue.pop();
		}
	}

	void Server::OnClientConnecting(ClientID clientID)
	{
	}

	void Server::OnClientConnected(ClientID clientID)
	{
		PT_CORE_INFO("Client id={} connected", clientID);
		m_NetStatistics->AllocateNetworkStatsBuffer(clientID);
		m_NetReplicator->Server_OnClientConnected(clientID);

		GameModeBase* gameMode = m_GameInstance->GetActiveScene()->GetGameMode();
		gameMode->Server_OnClientConnected(clientID);
	}

	void Server::OnClientDisconnected(ClientID clientID)
	{
		PT_CORE_INFO("Client id={} disconnected", clientID);
		m_NetStatistics->ReleaseNetworkStatsBuffer(clientID);

		GameModeBase* gameMode = m_GameInstance->GetActiveScene()->GetGameMode();
		gameMode->Server_OnClientDisconnected(clientID);
	}

	uint32_t Server::GetConnectedClientsCount() const
	{
		uint32_t count = 0;
		for (const auto& client : m_ConnectedClients)
		{
			const auto& clientInfo = client.second;
			if (clientInfo.Status == ConnectionStatus::Connected)
				count++;
		}
		return count;
	}

	void Server::SetClientEntity(ClientID clientID, Entity entity)
	{
		m_ClientToEntityMap[clientID] = entity;
	}

	Entity Server::GetClientEntity(ClientID clientID)
	{
		auto entityIt = m_ClientToEntityMap.find(clientID);
		if (entityIt != m_ClientToEntityMap.end())
			return entityIt->second;
		return Entity{};
	}

	void Server::SetClientName(ClientID clientID, const char* name)
	{
		auto& clientInfo = m_ConnectedClients.at(clientID);
		clientInfo.ClientName = name;
		m_Interface->SetConnectionName((HSteamNetConnection)clientID, name);
	}

	void Server::SetClientActionCallback(uint32_t clientID, StreamReaderDelegate function)
	{
		m_PlayerActionCallbacks[clientID] = function;
	}

	void Server::KickClient(ClientID clientID)
	{
		m_Interface->CloseConnection(clientID, 0, "Kicked by host", false);
	}

	void Server::OnEntitySpawned(Scene* scene, UUID entityUUID)
	{
		m_NetReplicator->Server_AddSpawnedEntity(scene, entityUUID);
	}

	void Server::OnEntityDespawned(Scene* scene, UUID entityUUID)
	{
		m_NetReplicator->Server_AddDespawnedEntity(scene, entityUUID);
	}

	void Server::SetPacketFakeLag(float latencyMs)
	{
		s_FakeServerLag = glm::max(latencyMs, 0.0f);
		float simulatedLatency = s_FakeServerLag / 4.0f;

		SteamNetworkingUtils()->SetGlobalConfigValueFloat(k_ESteamNetworkingConfig_FakePacketLag_Send, simulatedLatency);
		SteamNetworkingUtils()->SetGlobalConfigValueFloat(k_ESteamNetworkingConfig_FakePacketLag_Recv, simulatedLatency);
	}

	void Server::NetworkThreadFunction()
	{
		s_Instance = this;
		m_Running = true;

		SteamDatagramErrMsg errMsg;
		if (!GameNetworkingSockets_Init(nullptr, errMsg))
		{
			OnFatalError(fmt::format("GameNetworkingSockets_Init failed: {}", errMsg));
			return;
		}

		m_Interface = SteamNetworkingSockets();

		// Start listening
		SteamNetworkingIPAddr serverLocalAddress;
		serverLocalAddress.Clear();
		serverLocalAddress.m_port = m_NetworkManager->m_Port;

		SteamNetworkingConfigValue_t options;
		options.SetPtr(k_ESteamNetworkingConfig_Callback_ConnectionStatusChanged, (void*)Server::ConnectionStatusChangedCallback);

		// Try to start listen socket on port
		m_ListenSocket = m_Interface->CreateListenSocketIP(serverLocalAddress, 1, &options);
		if (m_ListenSocket == k_HSteamListenSocket_Invalid)
		{
			OnFatalError(fmt::format("Fatal error: Failed to listen on port {}", m_Port));
			return;
		}

		// Try to create poll group
		m_PollGroup = m_Interface->CreatePollGroup();
		if (m_PollGroup == k_HSteamNetPollGroup_Invalid)
		{
			OnFatalError(fmt::format("Fatal error: Failed to listen on port {}", m_Port));
			return;
		}

		PT_CORE_INFO("Server listening on port {}", m_Port);
		while (m_Running)
		{
			PollIncomingMessages();
			PollConnectionStateChanges();
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
		}

		// Close all the connections
		PT_CORE_INFO("Closing connections...");
		for (const auto& [clientID, clientInfo] : m_ConnectedClients)
		{
			m_Interface->CloseConnection(clientID, 0, "Server Shutdown", true);
		}

		m_ConnectedClients.clear();

		m_Interface->CloseListenSocket(m_ListenSocket);
		m_ListenSocket = k_HSteamListenSocket_Invalid;

		m_Interface->DestroyPollGroup(m_PollGroup);
		m_PollGroup = k_HSteamNetPollGroup_Invalid;

		m_NetworkThreadFinished = true;
	}

	void Server::ConnectionStatusChangedCallback(SteamNetConnectionStatusChangedCallback_t* info) 
	{ 
		s_Instance->OnConnectionStatusChanged(info); 
	}

	void Server::OnConnectionStatusChanged(SteamNetConnectionStatusChangedCallback_t* status)
	{
		// Handle connection state
		switch (status->m_info.m_eState)
		{
		case k_ESteamNetworkingConnectionState_None:
			// NOTE: We will get callbacks here when we destroy connections.  You can ignore these.
			break;

		case k_ESteamNetworkingConnectionState_ClosedByPeer:
		case k_ESteamNetworkingConnectionState_ProblemDetectedLocally:
		{
			// Ignore if they were not previously connected.  (If they disconnected
			// before we accepted the connection.)
			if (status->m_eOldState == k_ESteamNetworkingConnectionState_Connected)
			{
				// Locate the client.  Note that it should have been found, because this
				// is the only codepath where we remove clients (except on shutdown),
				// and connection change callbacks are dispatched in queue order.
				auto itClient = m_ConnectedClients.find(status->m_hConn);
				//assert(itClient != m_mapClients.end());

				// Either ClosedByPeer or ProblemDetectedLocally - should be communicated to user callback
				// User callback
				//m_ClientDisconnectedCallback(itClient->second);

				if (itClient != m_ConnectedClients.end())
				{
					if (itClient->second.Status == ConnectionStatus::Connected)
					{
						// Ignore disconnected connections if they ere not previously connected (failed to handshake).
						std::lock_guard<std::mutex> lock(m_ConnectionStatusChangeQueueMutex);
						m_ConnectionStatusChangeQueue.push(std::make_pair(itClient->second.ID, ConnectionStatus::Disconnected));
					}
					m_ConnectedClients.erase(itClient);
				}
			}
			else
			{
				//assert(info->m_eOldState == k_ESteamNetworkingConnectionState_Connecting);
			}

			// Clean up the connection. This is important!
			// The connection is "closed" in the network sense, but
			// it has not been destroyed. We must close it on our end, too
			// to finish up. The reason information do not matter in this case,
			// and we cannot linger because it's already closed on the other end,
			// so we just pass 0s.
			m_Interface->CloseConnection(status->m_hConn, 0, nullptr, false);
			break;
		}

		case k_ESteamNetworkingConnectionState_Connecting:
		{
			// This must be a new connection
			PT_CORE_ASSERT(m_ConnectedClients.find(info->m_hConn) == m_ConnectedClients.end());

			// Check if connections limit exceeded
			if (m_ConnectedClients.size() >= m_NetworkManager->m_MaxServerConnections)
			{
				m_Interface->CloseConnection(status->m_hConn, NetConnectionEndCode_MaxConnections, "Connections limit reached", false);
				PT_CORE_WARN("Blocked connection from client {}: Connections limit ({}) reached", status->m_hConn, m_NetworkManager->m_MaxServerConnections);
				break;
			}

			// Try to accept incoming connection
			if (m_Interface->AcceptConnection(status->m_hConn) != k_EResultOK)
			{
				m_Interface->CloseConnection(status->m_hConn, 0, nullptr, false);
				PT_CORE_ERROR("Couldn't accept connection");
				break;
			}

			// Assign the poll group
			if (!m_Interface->SetConnectionPollGroup(status->m_hConn, m_PollGroup))
			{
				m_Interface->CloseConnection(status->m_hConn, 0, nullptr, false);
				PT_CORE_ERROR("Failed to set poll group");
				break;
			}

			// Retrieve connection info
			SteamNetConnectionInfo_t connectionInfo;
			m_Interface->GetConnectionInfo(status->m_hConn, &connectionInfo);

			// Register connected client
			auto& clientInfo = m_ConnectedClients[status->m_hConn];
			clientInfo.ID = (ClientID)status->m_hConn;
			clientInfo.ConnectionDesc = connectionInfo.m_szConnectionDescription;
			clientInfo.Status = ConnectionStatus::Connecting;

			PT_CORE_INFO("New connection from client {}", clientInfo.ConnectionDesc);
			{
				std::lock_guard<std::mutex> lock(m_ConnectionStatusChangeQueueMutex);
				m_ConnectionStatusChangeQueue.push(std::make_pair(clientInfo.ID, clientInfo.Status));
			}

			break;
		}

		case k_ESteamNetworkingConnectionState_Connected:
			// We will get a callback immediately after accepting the connection.
			// Since we are the server, we can ignore this, it's not news to us.
			break;

		default:
			break;
		}
	}

	void Server::PollIncomingMessages()
	{
		// Process all messages
		while (m_Running)
		{
			ISteamNetworkingMessage* incomingMessage = nullptr;
			int messageCount = m_Interface->ReceiveMessagesOnPollGroup(m_PollGroup, &incomingMessage, 1);
			if (messageCount == 0)
				break;

			if (messageCount < 0)
			{
				// messageCount < 0 means critical error?
				m_Running = false;
				return;
			}

			// assert(numMsgs == 1 && pIncomingMsg);

			auto itClient = m_ConnectedClients.find(incomingMessage->m_conn);
			if (itClient == m_ConnectedClients.end())
			{
				PT_CORE_ERROR("Received data from unregistered client");
				continue;
			}

			if (incomingMessage->m_cbSize)
			{
				std::lock_guard<std::mutex> lock(m_MessageQueueMutex);
				m_MessageQueue.push(incomingMessage);
			}
		}
	}

	void Server::PollConnectionStateChanges()
	{
		m_Interface->RunCallbacks();
	}

	void Server::OnFatalError(const std::string& message)
	{
		PT_CORE_CRITICAL(message);
		m_Running = false;
	}

	void Server::SendBufferToClient(ClientID clientID, Buffer buffer, bool reliable)
	{
		m_Interface->SendMessageToConnection((HSteamNetConnection)clientID, buffer.Data, (ClientID)buffer.Size, reliable ? k_nSteamNetworkingSend_Reliable : k_nSteamNetworkingSend_Unreliable, nullptr);
	}

	void Server::SendBufferToAllClients(Buffer buffer, ClientID excludeClientID, bool reliable)
	{
		for (const auto& [clientID, clientInfo] : m_ConnectedClients)
		{
			if (clientInfo.Status == ConnectionStatus::Connected && clientID != excludeClientID)
				SendBufferToClient(clientID, buffer, reliable);
		}
	}

	void Server::SendStringToClient(ClientID clientID, const std::string& string, bool reliable)
	{
		SendBufferToClient(clientID, Buffer(string.data(), string.size()), reliable);
	}

	void Server::SendStringToAllClients(const std::string& string, ClientID excludeClientID, bool reliable)
	{
		SendBufferToAllClients(Buffer(string.data(), string.size()), excludeClientID, reliable);
	}

}
