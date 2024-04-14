#include "ptpch.h"
#include "Proton/Network/Server/Server.h"
#include "Proton/Network/Common/PacketType.h"
#include "Proton/Network/Common/NetworkManager.h"

#include "Proton/Core/GameInstance.h"
#include "Proton/Core/Timer.h"
#include "Proton/Assets/SceneSerializer.h"
#include "Proton/Scene/Scene.h"
#include "Proton/Scene/SceneManager.h"
#include "Proton/Scene/Entity.h"
#include "Proton/Scripting/GameModeBase.h"

#ifdef PT_EDITOR
#include "Proton/Editor/Panels/InfoPanel.h"
#endif

#include <chrono>
#include <iomanip>
#include <spdlog/spdlog.h>

#ifdef PROTON_DISTRIBUTION
static constexpr bool s_EnableNetworkStatistics = false;
#else
static constexpr bool s_EnableNetworkStatistics = true;
#endif


namespace proton {

	// 1 MB scratch buffer for writting messages
	constexpr static size_t s_ScratchBufferSize = 1048576;
	constexpr static size_t s_DetailedStatusBufferSize = 2048;

	// Can only have one server instance per-process
	static Server* s_Instance = nullptr;

	Server::Server(GameInstance* gameInstance)
		: m_GameInstance(gameInstance), m_NetworkManager(gameInstance->m_NetworkManager.get())
	{
		m_ScratchBuffer.Allocate(s_ScratchBufferSize);

		if constexpr (s_EnableNetworkStatistics)
			GenerateStatsLogsFilename();
	}

	Server::~Server()
	{
		if (m_NetworkThread.joinable())
			m_NetworkThread.join();

		m_ScratchBuffer.Release();

		for (auto& it : m_NetworkStats)
			it.second.Detailed.Release();
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
	////                        Game Server and Entity Replication Functionality                       ////
	///////////////////////////////////////////////////////////////////////////////////////////////////////

	void Server::MainThread_OnTick()
	{
		ProcessConnectionStatusQueue();
		ProcessMessages();
		SendReplicationData();

		if constexpr(s_EnableNetworkStatistics)
			UpdateNetworkStatistics();
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

	void Server::SetPlayerActionCallback(uint32_t clientID, OnRecvPlayerActionCallback function)
	{
		m_PlayerActionCallbacks[clientID] = function;
	}

	void Server::QueueAddCreatedEntity(Entity entity, ClientID specificClient)
	{
		m_OnCreatedEntityQueue.push({ entity, specificClient });
	}

	void Server::OnEntityCreated(Entity entity, ClientID specificClient)
	{
		BufferStreamWriter stream(m_ScratchBuffer);
		stream.WriteRaw(PacketType::EntitySpawn);
		SceneSerializer serializer(entity.GetScene());
		std::string jsonData = serializer.SerializeEntityToString(entity);
		stream.WriteString(jsonData);

		Buffer buffer = Buffer(m_ScratchBuffer, stream.GetStreamPosition());
		if (specificClient)
			SendBufferToClient(specificClient, buffer);
		else
			SendBufferToAllClients(buffer);
	}

	void Server::OnEntityDestroyed(Entity entity, ClientID specificClient)
	{
		BufferStreamWriter stream(m_ScratchBuffer);
		stream.WriteRaw(PacketType::EntityDestroy);
		stream.WriteRaw(entity.GetUUID());

		Buffer buffer = Buffer(m_ScratchBuffer, stream.GetStreamPosition());
		if (specificClient)
			SendBufferToClient(specificClient, buffer);
		else
			SendBufferToAllClients(buffer);
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

				if constexpr (s_EnableNetworkStatistics)
					InitNetworkStatsForClient(info.ClientInfo.ID);
			}

			// Client has been disconnected
			else if (info.Status == ConnectionStatus::Disconnected)
			{
				GameModeBase* gameMode = m_GameInstance->GetActiveScene()->GetGameMode();
				gameMode->Server_OnClientDisconnected(info.ClientInfo.ID);

				// Client network statistics
				if constexpr (s_EnableNetworkStatistics)
					ReleaseNetworkStatsForClient(info.ClientInfo.ID);
			}

			m_ClientConnStatusChangeQueue.pop();
		}
	}

	void Server::ProcessMessages()
	{
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

			case PacketType::VerifyGameState:
			{
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
					SceneSerializer serializer(entity.GetScene());
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

				uint64_t streamEnd = writer.GetStreamPosition();
				writer.SetStreamPosition(sizeof(PacketType));
				writer.WriteRaw(created);
				writer.WriteRaw(destroyed);
				SendBufferToClient(message->GetConnection(), Buffer(m_ScratchBuffer, streamEnd));

				PT_CORE_TRACE("VerifyGameState: client_id={}, created={}, destroyed={}", message->GetConnection(), created, destroyed);
				break;
			}

			case PacketType::PlayerAction:
			{
				if (m_PlayerActionCallbacks.find(message->m_conn) != m_PlayerActionCallbacks.end())
				{
					OnRecvPlayerActionCallback& callback = m_PlayerActionCallbacks.at(message->m_conn);
					callback(stream);
				}
				else
					PT_CORE_ERROR("PlayerActionCallback was not defined! client_id={}", (uint32_t)message->m_conn);
				
				break;
			}

			default:
				PT_CORE_ERROR("Invalid packet type ({}).", (uint16_t)packetType);
				// TODO: Kick client
				break;
			}

			message->Release();
			m_MessageQueue.pop();
		}
	}

	void Server::SendReplicationData()
	{
		Scene* scene = m_GameInstance->GetActiveScene();

		// Process created entities queue
		while (!m_OnCreatedEntityQueue.empty())
		{
			EntityQueueEntry& entry = m_OnCreatedEntityQueue.front();
			
			if (entry.entity.HasComponent<NetworkComponent>())
				OnEntityCreated(entry.entity, entry.client);
			
			m_OnCreatedEntityQueue.pop();
		}

		// Replicate entitites with NetworkComponent
		auto view = scene->GetAllEntitiesWith<NetworkComponent>();
		
		if (view.empty())
			return;
		
		BufferStreamWriter stream(m_ScratchBuffer);
		stream.WriteRaw(PacketType::UpdateReplicated);

		for (entt::entity e : view)
		{
			Entity entity(e, scene);
			auto& net = view.get<NetworkComponent>(e);

			uint64_t entityStreamStart = stream.GetStreamPosition();
			uint64_t entityBufferSize = 0;

			stream.WriteZero(sizeof(entityBufferSize));
			stream.WriteRaw(entity.GetUUID());

			if (net.IsReplicated(ComponentTypeID::Transform))
			{
				auto& transform = entity.GetTransform();
				stream.WriteRaw(transform);
			}

			if (net.IsReplicated(ComponentTypeID::Sprite))
			{
				auto& sprite = entity.GetSprite();
				stream.WriteRaw(sprite.GetTilePos());
				stream.WriteRaw(sprite.GetMirrorFlip());
			}

			uint64_t entityStreamEnd = stream.GetStreamPosition();
			entityBufferSize = entityStreamEnd - entityStreamStart;
				
			stream.SetStreamPosition(entityStreamStart);
			stream.WriteRaw(entityBufferSize);
			stream.SetStreamPosition(entityStreamEnd);

			if constexpr (s_EnableNetworkStatistics)
				m_ReplicationStats.RepEntitiesCount++;
		}

		if constexpr (s_EnableNetworkStatistics)
			m_ReplicationStats.RepPacketCount++;

		SendBufferToAllClients(Buffer(m_ScratchBuffer, stream.GetStreamPosition()));
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////
	////                           Network Traffic and Replication Statistics                          ////
	///////////////////////////////////////////////////////////////////////////////////////////////////////

	void Server::InitNetworkStatsForClient(ClientID clientID)
	{
		Buffer detailedConnStatusBuffer;
		detailedConnStatusBuffer.Allocate(s_DetailedStatusBufferSize);
		m_NetworkStats[clientID].Detailed = detailedConnStatusBuffer;

		if (m_NetworkManager->m_SaveStatsForClientID == 0)
		{
			m_NetworkManager->m_SaveStatsForClientID = clientID;
		}
	}

	void Server::ReleaseNetworkStatsForClient(ClientID clientID)
	{
		m_NetworkStats.at(clientID).Detailed.Release();
		m_NetworkStats.erase(clientID);

		if (m_NetworkManager->m_SaveStatsForClientID == clientID)
		{
			m_NetworkManager->m_SaveStatsForClientID =
				m_ConnectedClients.size() ? m_ConnectedClients.begin()->first : 0;
		}
	}

	void Server::UpdateNetworkStatistics()
	{
		static Timer timer;
		if (timer.Elapsed() >= m_StatsUpdateInterval)
		{
			for (auto& [hConn, stats] : m_NetworkStats)
			{
				m_Interface->GetConnectionRealTimeStatus(hConn, &stats.RealTime, 0, NULL);
				m_Interface->GetDetailedConnectionStatus(hConn, (char*)stats.Detailed.Data, s_DetailedStatusBufferSize);

				NetworkManager* m = m_NetworkManager;
				if (m->m_SaveNetworkStatsToLogFile && (hConn == m->m_SaveStatsForClientID || m->m_SaveStatsForAllClients))
				{
					SaveStatsLogsToFile(hConn, stats.RealTime);
				}
			}
			timer.Reset();
		}
	}

	static inline double round(float f)
	{
		return std::round((double)f * 100000) / 100000;
	}

	void Server::SaveStatsLogsToFile(ClientID clientID, SteamNetConnectionRealTimeStatus_t& status)
	{
		int in = (int)status.m_flInBytesPerSec;
		int out = (int)status.m_flOutBytesPerSec;
		int ping = status.m_nPing;
		int replicated = m_ReplicationStats.RepEntitiesCount / m_ReplicationStats.RepPacketCount;
		m_ReplicationStats = { 0 , 0 };

		if (in == 0 && out == 0)
			return;

		static Timer timer;

		std::ofstream logFile("logs/" + m_StatsLogsFilename, std::ios_base::app);
		if (!m_StatsLogsHeaderWritten)
		{
			// Write header to log file
			logFile << "# scene=" << m_GameInstance->GetActiveScene()->GetFilepath() << "\r\n";
			logFile << "# tick_rate=" << m_NetworkManager->m_ServerTickRate << "\r\n";
			logFile << "# client_id; timepoint; in_bps; out_bps; ping; rep_count\r\n";
			m_StatsLogsHeaderWritten = true;
			timer.Reset();
		}

		logFile  << clientID << "; " 
			<< round(timer.Elapsed()) << "; "
			<< in << "; " 
			<< out << "; " 
			<< ping << "; "
			<< replicated << "\r\n";

		logFile.close();
	}

	void Server::GenerateStatsLogsFilename()
	{
		auto now = std::chrono::system_clock::now();
		auto in_time_t = std::chrono::system_clock::to_time_t(now);

		std::tm bt{};
		localtime_s(&bt, &in_time_t);

		std::ostringstream oss;
		oss << "NetworkStats_" << std::put_time(&bt, "%Y%m%d_%H%M%S") << ".log";
		m_StatsLogsFilename = oss.str();
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

			PT_CORE_INFO("New connection from client {}", client.ID);
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

	void Server::SetClientNick(HSteamNetConnection hConn, const char* nick)
	{
		// Set the connection name, too, which is useful for debugging
		m_Interface->SetConnectionName(hConn, nick);
	}

	void Server::KickClient(ClientID clientID)
	{
		m_Interface->CloseConnection(clientID, 0, "Kicked by host", false);
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
