#include "ptpch.h"
#include "Proton/Network/Server.h"
#include "Proton/Network/Packets.h"
#include "Proton/Network/NetworkManager.h"
#include "Proton/Network/ReplicationManager.h"
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

//#define _DEBUG_NO_COMPONENT_CHECKSUM_VERIFY

namespace proton {

	// 1 MB scratch buffer for writting messages
	constexpr static size_t s_ScratchBufferSize = 1048576;

	// Can only have one server instance per-process
	Server* Server::s_Instance = nullptr;
	float Server::s_FakeServerLag = 0.0f;

	Server::Server(GameInstance* gameInstance)
		: m_GameInstance(gameInstance), m_NetworkManager(gameInstance->m_NetworkManager.get()),
		m_ReplicationManager(MakeUnique<ReplicationManager>(this)),
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

	void Server::Start(int port)
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

	///////////////////////////////////////////////////////////////////////////////////////////////////////
	////                                   Game Server Functionality                                   ////
	///////////////////////////////////////////////////////////////////////////////////////////////////////

	void Server::MainThread_OnTick()
	{
		Scene* scene = m_GameInstance->GetActiveScene();

		ProcessConnectionStatusQueue();
		ProcessClientMessagesQueue();
		ProcessSpawnedEntityQueue(scene);
		ProcessDespawnedEntityQueue(scene);

	#ifdef _DEBUG_NO_COMPONENT_CHECKSUM_VERIFY
		SendReplicationUpdate(scene, 0, false);
	#else
		SendReplicationUpdate(scene);
	#endif
		
		m_NetStatistics->UpdateNetworkStatistics();
	}

	void Server::ProcessConnectionStatusQueue()
	{
		std::lock_guard<std::mutex> lock(m_ClientConnStatusQueueMutex);
		while (!m_ClientConnStatusChangeQueue.empty())
		{
			ClientConnectionStatusChangeInfo& info = m_ClientConnStatusChangeQueue.front();

			if (info.Status == ConnectionStatus::Connecting)
			{
				m_NetStatistics->AllocateNetworkStatsBuffer(info.ClientInfo.ID);
				// Wait for PacketType::Handshake from client 
				// to change status to ConnectionStatus::Connected
			}

			// Client has been disconnected
			else if (info.Status == ConnectionStatus::Disconnected)
			{
				GameModeBase* gameMode = m_GameInstance->GetActiveScene()->GetGameMode();
				gameMode->Server_OnClientDisconnected(info.ClientInfo.ID);

				m_NetStatistics->ReleaseNetworkStatsBuffer(info.ClientInfo.ID);
			}

			m_ClientConnStatusChangeQueue.pop();
		}
	}

	void Server::ProcessClientMessagesQueue()
	{
		PROFILE_FUNCTION();

		std::lock_guard<std::mutex> lock(m_MessageQueueMutex);
		//SceneManager* sceneManager = m_GameInstance->GetSceneManager();
		Scene* scene = m_GameInstance->GetActiveScene();

		// Kick clients which failed handshake verification
		for (auto& kv : m_ConnectedClients)
		{
			auto& clientInfo = kv.second;
			if (clientInfo.Status == ConnectionStatus::FailedToConnect)
				KickClient(clientInfo.ID);
		}

		while (!m_MessageQueue.empty())
		{
			ISteamNetworkingMessage* message = m_MessageQueue.front();
			uint32_t clientID = message->GetConnection();

			Buffer buffer(message->m_pData, message->m_cbSize);
			BufferStreamReader stream(buffer);

			BufferStreamWriter writer(m_ScratchBuffer);
			
			PacketType packetType;
			stream.ReadRaw(packetType);
			stream.SetStreamPosition(0);

			switch (packetType)
			{
			////////////////////////////////////////////////////////////////////////////////////////////////////
			case PacketType::Handshake:
			{
				NetMessageHandshake handshake;
				stream.ReadRaw(handshake);

				NetMessageHandshakeReply reply;
				reply.ResultCode = 0;
				reply.ClientID = clientID;

				if (handshake.EngineProtocolVersion != PT_NET_PROTOCOL_VERSION
					&& handshake.GameProtocolVersion != NetworkManager::s_GameProtocolVersion)
				{
					reply.ResultCode = 3;
				}
				else if (handshake.GameProtocolVersion != NetworkManager::s_GameProtocolVersion)
				{
					reply.ResultCode = 2;
				}
				else if (handshake.EngineProtocolVersion != PT_NET_PROTOCOL_VERSION)
				{
					reply.ResultCode = 1;
				}
				
				writer.WriteRaw(reply);
				SendBufferToClient(clientID, writer.GetBuffer());

				auto& clientInfo = m_ConnectedClients[clientID];
				if (reply.ResultCode == 0)
				{
					PT_CORE_INFO("Client id={} connected", clientID);
					clientInfo.Status = ConnectionStatus::Connected;

					SendAllSpawnedEntities(scene, clientID);
					SendAllDespawnedEntities(scene, clientID);
					SendReplicationUpdate(scene, clientID, false);

					GameModeBase* gameMode = m_GameInstance->GetActiveScene()->GetGameMode();
					gameMode->Server_OnClientConnected(message->m_conn);
				}
				else
				{
					PT_CORE_WARN("Blocked connection id={} with result={}", clientID, reply.ResultCode);
					clientInfo.Status = ConnectionStatus::FailedToConnect;
				}

				break;
			}

			////////////////////////////////////////////////////////////////////////////////////////////////////

			case PacketType::PlayerAction:
			{
				PROFILE_SCOPE("PacketType::PlayerAction");

				stream.SkipBytes(sizeof(NetMassagePlayerAction));
				if (m_PlayerActionCallbacks.find(clientID) != m_PlayerActionCallbacks.end())
				{
					StreamReaderDelegate& callback = m_PlayerActionCallbacks.at(clientID);
					callback(stream);
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

			message->Release();
			m_MessageQueue.pop();
		}
	}

	void Server::AddSpawnedEntity(Scene* scene, UUID entityUUID)
	{
		SceneData& sceneData = m_SceneData[scene];
		sceneData.SpawnedEntityQueue.push(entityUUID);
	}

	void Server::AddDespawnedEntity(Scene* scene, UUID entityUUID)
	{
		SceneData& sceneData = m_SceneData[scene];
		sceneData.DespawnedEntityQueue.push(entityUUID);
	}

	void Server::ProcessSpawnedEntityQueue(Scene* scene)
	{
		PROFILE_FUNCTION();

		SceneData& sceneData = m_SceneData[scene];

		if (sceneData.SpawnedEntityQueue.empty())
			return;

		NetMessageSpawn msg;
		BufferStreamWriter stream(m_ScratchBuffer);
		stream.SkipBytes(sizeof(NetMessageSpawn));

		uint32_t spawned = 0;

		while (!sceneData.SpawnedEntityQueue.empty())
		{
			UUID uuid = sceneData.SpawnedEntityQueue.front();
			Entity entity = scene->FindByID(uuid);

			if (!entity.HasComponent<NetworkComponent>())
			{
				sceneData.SpawnedEntityQueue.pop();
				continue;
			}

			SceneSerializer serializer(entity.m_Scene, true);
			stream.WriteString(serializer.SerializeEntityToString(entity));
			spawned++;

			sceneData.SpawnedEntityQueue.pop();
			sceneData.SpawnedAll.push_back(uuid);
		}

		if (spawned > 0)
		{
			msg.EntityCount = spawned;
			uint64_t streamPos = stream.GetStreamPosition();
			stream.SetStreamPosition(0);
			stream.WriteRaw(msg);
			stream.SetStreamPosition(streamPos);
			SendBufferToAllClients(Buffer(m_ScratchBuffer, stream.GetStreamPosition())); 
		}
	}

	void Server::ProcessDespawnedEntityQueue(Scene* scene)
	{
		PROFILE_FUNCTION();

		SceneData& sceneData = m_SceneData[scene];

		if (sceneData.DespawnedEntityQueue.empty())
			return;

		NetMessageDespawn msg;
		BufferStreamWriter stream(m_ScratchBuffer);
		stream.SkipBytes(sizeof(NetMessageDespawn));

		uint32_t despawned = 0;

		while (!sceneData.DespawnedEntityQueue.empty())
		{
			UUID uuid = sceneData.DespawnedEntityQueue.front();

			stream.WriteRaw(uuid);
			despawned++;

			sceneData.DespawnedEntityQueue.pop();

			auto& spawnedAll = sceneData.SpawnedAll;
			auto spawnedIt = std::find(spawnedAll.begin(), spawnedAll.end(), uuid);
			
			if (spawnedIt != spawnedAll.end())
				spawnedAll.erase(spawnedIt);
			else
				sceneData.DespawnedAll.push_back(uuid);
		}

		if (despawned > 0)
		{
			msg.EntityCount = despawned;
			uint64_t streamPos = stream.GetStreamPosition();
			stream.SetStreamPosition(0);
			stream.WriteRaw(msg);
			stream.SetStreamPosition(streamPos);
			SendBufferToAllClients(stream.GetBuffer());
		}
	}

	void Server::SendAllSpawnedEntities(Scene* scene, ClientID clientID)
	{
		SceneData& sceneData = m_SceneData[scene];
		auto& spawnedAll = sceneData.SpawnedAll;
		if (spawnedAll.size() > 0)
		{
			BufferStreamWriter stream(m_ScratchBuffer);

			NetMessageSpawn msg;
			msg.EntityCount = (uint32_t)spawnedAll.size();
			stream.WriteRaw(msg);

			for (UUID uuid : spawnedAll)
			{
				Entity entity = scene->FindByID(uuid);
				SceneSerializer serializer(scene, true);
				stream.WriteString(serializer.SerializeEntityToString(entity));
			}

			SendBufferToClient(clientID, stream.GetBuffer());
		}
	}

	void Server::SendAllDespawnedEntities(Scene* scene, ClientID clientID)
	{
		SceneData& sceneData = m_SceneData[scene];
		auto& despawnedAll = sceneData.DespawnedAll;
		if (despawnedAll.size() > 0)
		{
			BufferStreamWriter stream(m_ScratchBuffer);

			NetMessageDespawn msg;
			msg.EntityCount = (uint32_t)despawnedAll.size();
			stream.WriteRaw(msg);

			for (UUID uuid : despawnedAll)
			{
				stream.WriteRaw(uuid);
			}

			SendBufferToClient(clientID, stream.GetBuffer());
		}
	}

	void Server::SendReplicationUpdate(Scene* scene, ClientID clientID, bool verifyComponentChecksum)
	{
		PROFILE_FUNCTION();
		
		// Create buffer stream writer
		BufferStreamWriter stream(m_ScratchBuffer);
		uint64_t packetStreamStart = stream.GetStreamPosition();

		m_ReplicationManager->StreamWriteReplicationData(stream, scene, verifyComponentChecksum);

		// If anything was written, send buffer to clients
		if (stream.GetStreamPosition() >= packetStreamStart + sizeof(NetMessageReplicate))
		{
			if (clientID == 0)
				SendBufferToAllClients(Buffer(m_ScratchBuffer, stream.GetStreamPosition()));
			else
				SendBufferToClient(clientID, Buffer(m_ScratchBuffer, stream.GetStreamPosition()));
		}

		m_NetStatistics->m_ReplicationStats.RepPacketCount++;
	}

	void Server::OnClientConnected(const ClientInfo& clientInfo) // Called from Network Thread
	{
		std::lock_guard<std::mutex> lock(m_ClientConnStatusQueueMutex);
		m_ClientConnStatusChangeQueue.push({ clientInfo, ConnectionStatus::Connecting });
	}

	void Server::OnClientDisconnected(const ClientInfo& clientInfo) // Called from Network Thread
	{
		std::lock_guard<std::mutex> lock(m_ClientConnStatusQueueMutex);
		m_ClientConnStatusChangeQueue.push({ clientInfo, ConnectionStatus::Disconnected });
	}

	void Server::OnDataReceived(ISteamNetworkingMessage* incomingMessage) // Called from Network Thread
	{
		std::lock_guard<std::mutex> lock(m_MessageQueueMutex);
		m_MessageQueue.push(incomingMessage);
	}

	void Server::SetClientActionCallback(uint32_t clientID, StreamReaderDelegate function)
	{
		m_PlayerActionCallbacks[clientID] = function;
	}

	void Server::SetClientNick(HSteamNetConnection hConn, const char* nick)
	{
		// Set the connection name, too, which is useful for debugging
		m_Interface->SetConnectionName(hConn, nick);
	}

	void Server::KickClient(ClientID clientID)
	{
		m_Interface->CloseConnection(clientID, 0, "Kicked by host", false);
	}

	void Server::SetPacketFakeLag(float latencyMs)
	{
		s_FakeServerLag = glm::max(latencyMs, 0.0f);

		float simulatedLatency = s_FakeServerLag / 4.0f;

		SteamNetworkingUtils()->SetGlobalConfigValueFloat(k_ESteamNetworkingConfig_FakePacketLag_Send, simulatedLatency);
		SteamNetworkingUtils()->SetGlobalConfigValueFloat(k_ESteamNetworkingConfig_FakePacketLag_Recv, simulatedLatency);
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////
	////               Network Thread and GameNetworkingSockets Interface Implementation               ////
	///////////////////////////////////////////////////////////////////////////////////////////////////////

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

				PT_CORE_INFO("Client {} disconnected", itClient->second.ID);
				OnClientDisconnected(itClient->second);

				m_ConnectedClients.erase(itClient);
			}
			else
			{
				//assert(info->m_eOldState == k_ESteamNetworkingConnectionState_Connecting);
			}

			// Clean up the connection.  This is important!
			// The connection is "closed" in the network sense, but
			// it has not been destroyed.  We must close it on our end, too
			// to finish up.  The reason information do not matter in this case,
			// and we cannot linger because it's already closed on the other end,
			// so we just pass 0s.
			m_Interface->CloseConnection(status->m_hConn, 0, nullptr, false);
			break;
		}

		case k_ESteamNetworkingConnectionState_Connecting:
		{
			// This must be a new connection
			// assert(m_mapClients.find(info->m_hConn) == m_mapClients.end());

			// Try to accept incoming connection
			if (m_Interface->AcceptConnection(status->m_hConn) != k_EResultOK)
			{
				m_Interface->CloseConnection(status->m_hConn, 0, nullptr, false);
				PT_CORE_ERROR("Couldn't accept connection (it was already closed?)");
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
			auto& client = m_ConnectedClients[status->m_hConn];
			client.ID = (ClientID)status->m_hConn;
			client.ConnectionDesc = connectionInfo.m_szConnectionDescription;
			client.Status = ConnectionStatus::Connecting;

			PT_CORE_INFO("New connection from client {}", client.ConnectionDesc);
			OnClientConnected(client);

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
				OnDataReceived(incomingMessage);
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
