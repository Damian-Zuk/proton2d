#include "ptpch.h"
#include "Proton/Network/Server/Server.h"
#include "Proton/Network/Common/PacketType.h"
#include "Proton/Network/Common/NetworkManager.h"
#include "Proton/Network/Server/ReplicationManager.h"
#include "Proton/Network/Server/NetStatsManager.h"

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
	static Server* s_Instance = nullptr;
	float Server::s_FakeServerLag = 0.0f;

	Server::Server(GameInstance* gameInstance)
		: m_GameInstance(gameInstance), m_NetworkManager(gameInstance->m_NetworkManager.get()),
		m_ReplicationManager(MakeUnique<ReplicationManager>(this)),
		m_NetStatsManager(MakeUnique<NetStatsManager>(this))
	{
		m_ScratchBuffer.Allocate(s_ScratchBufferSize);
		SetPacketFakeLag(s_FakeServerLag);
	}

	Server::~Server()
	{
		if (m_NetworkThread.joinable())
			m_NetworkThread.join();

		m_ScratchBuffer.Release();
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
		ProcessConnectionStatusQueue();
		ProcessClientMessagesQueue();
		ProcessCreatedEntityQueue();
		ProcessDestroyedEntityQueue();

	#ifdef _DEBUG_NO_COMPONENT_CHECKSUM_VERIFY
		SendReplicationUpdate(m_GameInstance->GetActiveScene(), 0, false);
	#else
		SendReplicationUpdate(m_GameInstance->GetActiveScene());
	#endif
		
		m_NetStatsManager->UpdateNetworkStatistics();
	}

	void Server::ProcessConnectionStatusQueue()
	{
		std::lock_guard<std::mutex> lock(m_ClientConnStatusQueueMutex);
		while (!m_ClientConnStatusChangeQueue.empty())
		{
			ClientConnectionStatusChangeInfo& info = m_ClientConnStatusChangeQueue.front();

			// Client has been connected
			if (info.Status == ConnectionStatus::Connected)
			{
				BufferStreamWriter stream(m_ScratchBuffer);
				stream.WriteRaw(PacketType::ConnectionAccepted);
				stream.WriteRaw(info.ClientInfo.ID);
				SendBufferToClient(info.ClientInfo.ID, Buffer(m_ScratchBuffer, stream.GetStreamPosition()));

				GameModeBase* gameMode = m_GameInstance->GetActiveScene()->GetGameMode();
				gameMode->Server_OnClientConnected(info.ClientInfo.ID);

				m_NetStatsManager->AllocateNetworkStatsBuffer(info.ClientInfo.ID);
			}

			// Client has been disconnected
			else if (info.Status == ConnectionStatus::Disconnected)
			{
				GameModeBase* gameMode = m_GameInstance->GetActiveScene()->GetGameMode();
				gameMode->Server_OnClientDisconnected(info.ClientInfo.ID);

				m_NetStatsManager->ReleaseNetworkStatsBuffer(info.ClientInfo.ID);
			}

			m_ClientConnStatusChangeQueue.pop();
		}
	}

	void Server::ProcessClientMessagesQueue()
	{
		PROFILE_FUNCTION();

		std::lock_guard<std::mutex> lock(m_MessageQueueMutex);
		SceneManager* sceneManager = m_GameInstance->GetSceneManager();
		Scene* scene = sceneManager->GetActiveScene();

		while (!m_MessageQueue.empty())
		{
			ISteamNetworkingMessage* message = m_MessageQueue.front();
			Buffer buffer(message->m_pData, message->m_cbSize);
			BufferStreamReader stream(buffer);
			PacketType packetType;
			stream.ReadRaw(packetType);

			switch (packetType)
			{
			////////////////////////////////////////////////////////////////////////////////////////////////////

			case PacketType::VerifyGameState:
			{
				PROFILE_SCOPE("PacketType::VerifyGameState");

				std::vector<UUID> clientState;
				std::vector<UUID> serverState;

				while (stream.GetStreamPosition() < buffer.Size)
				{
					uint64_t id = 0;
					stream.ReadRaw(id);
					clientState.push_back(id);
				}

				auto view = scene->GetAllEntitiesWith<IDComponent, NetworkComponent>();
				for (entt::entity e : view)
				{
					auto& id = view.get<IDComponent>(e);
					serverState.push_back(id.ID);
				}

				uint32_t created = 0, destroyed = 0;

				BufferStreamWriter writer(m_ScratchBuffer);
				writer.WriteRaw(PacketType::GameStateUpdate);
				writer.WriteZero(sizeof(uint32_t) * 2);

				for (const auto& uuid : serverState)
				{
					if (std::find(clientState.begin(), clientState.end(), uuid) != clientState.end())
						continue;

					Entity entity = scene->FindByID(uuid);
					SceneSerializer serializer(entity.GetScene(), true);
					std::string jsonData = serializer.SerializeEntityToString(entity);
					writer.WriteString(jsonData);
					created++;
				}

				for (const auto& uuid : clientState)
				{
					if (std::find(serverState.begin(), serverState.end(), uuid) != serverState.end())
						continue;

					writer.WriteRaw(uuid);
					destroyed++;
				}

				uint64_t streamPos = writer.GetStreamPosition();
				writer.SetStreamPosition(sizeof(PacketType));
				writer.WriteRaw(created);
				writer.WriteRaw(destroyed);
				writer.SetStreamPosition(streamPos);

				m_ReplicationManager->StreamWriteReplicationData(writer, scene, false);
				uint64_t streamEnd = writer.GetStreamPosition();
				PT_CORE_TRACE("SendRepSync: end={}, size={}", streamPos, streamEnd);
				
				SendBufferToClient(message->GetConnection(), Buffer(m_ScratchBuffer, streamEnd));

				PT_CORE_TRACE("VerifyGameState: client_id={}, created={}, destroyed={}", message->GetConnection(), created, destroyed);
				break;
			}

			////////////////////////////////////////////////////////////////////////////////////////////////////

			case PacketType::PlayerAction:
			{
				PROFILE_SCOPE("PacketType::PlayerAction");

				if (m_PlayerActionCallbacks.find(message->m_conn) != m_PlayerActionCallbacks.end())
				{
					StreamReaderDelegate& callback = m_PlayerActionCallbacks.at(message->m_conn);
					callback(stream);
				}
				else
					PT_CORE_ERROR("PlayerActionCallback was not defined! client_id={}", (uint32_t)message->m_conn);
				
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

	void Server::ProcessCreatedEntityQueue()
	{
		PROFILE_FUNCTION();

		BufferStreamWriter stream(m_ScratchBuffer);
		stream.WriteRaw(PacketType::EntitySpawn);

		std::unordered_map<ClientID, std::vector<Entity>> specificClients;
		bool anyCreated = false;

		while (!m_CreatedEntityQueue.empty())
		{
			EntityLifetimeQueueEntry& entry = m_CreatedEntityQueue.front();

			Entity entity = entry.Scene->FindByID(entry.EntityUUID);
			if (!entity.HasComponent<NetworkComponent>())
			{
				m_CreatedEntityQueue.pop();
				continue;
			}

			if (entry.Client != 0)
			{
				specificClients[entry.Client].push_back(entity);
				m_CreatedEntityQueue.pop();
				continue;
			}

			SceneSerializer serializer(entry.Scene, true);
			stream.WriteString(serializer.SerializeEntityToString(entity));
			anyCreated = true;
			
			m_CreatedEntityQueue.pop();
		}

		if (anyCreated)
			SendBufferToAllClients(Buffer(m_ScratchBuffer, stream.GetStreamPosition())); 

		for (auto& [clientID, entityVector] : specificClients)
		{
			stream.SetStreamPosition(0);
			stream.WriteRaw(PacketType::EntitySpawn);

			for (auto entity : entityVector)
			{
				SceneSerializer serializer(entity.GetScene(), true);
				stream.WriteString(serializer.SerializeEntityToString(entity));
			}

			SendBufferToClient(clientID, Buffer(m_ScratchBuffer, stream.GetStreamPosition()));
		}
	}

	void Server::ProcessDestroyedEntityQueue()
	{
		PROFILE_FUNCTION();

		BufferStreamWriter stream(m_ScratchBuffer);
		stream.WriteRaw(PacketType::EntityDestroy);

		std::unordered_map<ClientID, std::vector<UUID>> specificClients;
		bool anyDestroyed = false;

		while (!m_DestroyedEntityQueue.empty())
		{
			EntityLifetimeQueueEntry& entry = m_DestroyedEntityQueue.front();

			if (entry.Client != 0)
			{
				specificClients[entry.Client].push_back(entry.EntityUUID);
				m_DestroyedEntityQueue.pop();
				continue;
			}

			stream.WriteRaw(entry.EntityUUID);
			anyDestroyed = true;

			m_DestroyedEntityQueue.pop();
		}

		if (anyDestroyed)
			SendBufferToAllClients(Buffer(m_ScratchBuffer, stream.GetStreamPosition()));

		for (auto& [clientID, uuidVector] : specificClients)
		{
			stream.SetStreamPosition(0);
			stream.WriteRaw(PacketType::EntityDestroy);

			for (auto uuid : uuidVector)
				stream.WriteRaw(uuid);

			SendBufferToClient(clientID, Buffer(m_ScratchBuffer, stream.GetStreamPosition()));
		}
	}

	void Server::SendReplicationUpdate(Scene* scene, ClientID clientID, bool verifyComponentChecksum)
	{
		PROFILE_FUNCTION();
		
		// Create buffer stream writer
		BufferStreamWriter stream(m_ScratchBuffer);
		stream.WriteRaw(PacketType::UpdateReplicated);

		uint64_t packetStreamStart = stream.GetStreamPosition();
		m_ReplicationManager->StreamWriteReplicationData(stream, scene, verifyComponentChecksum);

		// If anything was written, send buffer to clients
		if (stream.GetStreamPosition() > packetStreamStart)
		{
			if (clientID == 0)
				SendBufferToAllClients(Buffer(m_ScratchBuffer, stream.GetStreamPosition()));
			else
				SendBufferToClient(clientID, Buffer(m_ScratchBuffer, stream.GetStreamPosition()));
		}

		m_NetStatsManager->m_ReplicationStats.RepPacketCount++;
	}

	void Server::OnClientConnected(const ClientInfo& clientInfo) // Called from Network Thread
	{
		std::lock_guard<std::mutex> lock(m_ClientConnStatusQueueMutex);
		m_ClientConnStatusChangeQueue.push({ clientInfo, ConnectionStatus::Connected });
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

	void Server::PushCreatedEntity(UUID entityUUID, Scene* scene, ClientID specificClient)
	{
		m_CreatedEntityQueue.push({ entityUUID, scene, specificClient });
	}

	void Server::PushDestroyedEntity(UUID entityUUID, Scene* scene, ClientID specificClient)
	{
		m_DestroyedEntityQueue.push({ entityUUID, scene, specificClient });
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
		m_FakePacketLag = glm::max(latencyMs, 0.0f);

		float simulatedLatency = m_FakePacketLag / 4.0f;

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
		serverLocalAddress.m_port = m_Port;

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
			if (clientID != excludeClientID)
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
