#include "ptpch.h"
#include "Proton/Network/Server/Server.h"
#include "Proton/Network/Common/PacketType.h"

#include "Proton/Core/GameInstance.h"
#include "Proton/Assets/SceneSerializer.h"
#include "Proton/Scene/Scene.h"
#include "Proton/Scene/Components.h"
#include "Proton/Scene/Entity.h"
#include "Proton/Scripting/GameModeBase.h"

#include <chrono>

#include <spdlog/spdlog.h>

namespace proton {

	// Can only have one server instance per-process
	static Server* s_Instance = nullptr;

	Server::Server(GameInstance* gameInstance)
		: m_GameInstance(gameInstance), m_NetworkManager(gameInstance->m_NetworkManager.get())
	{
		// 256KB scratch buffer
		m_ScratchBuffer.Allocate(256 * 1024);
	}

	Server::~Server()
	{
		if (m_NetworkThread.joinable())
			m_NetworkThread.join();

		m_ScratchBuffer.Release();
	}

	void Server::OnUpdate(float ts)
	{
	}

	void Server::Start(int port)
	{
		if (m_Running)
			return;

		m_Port = port;
		m_NetworkThread = std::thread([this]() { NetworkThreadFunc(); });
	}

	void Server::Stop()
	{
		m_Running = false;
	}

	void Server::SetOnRecvPlayerActionCallback(uint32_t clientID, OnRecvPlayerActionCallback function)
	{
		m_OnRecvPlayerActionCallbacks[clientID] = function;
	}

	void Server::OnEntityCreated(Entity entity)
	{
		BufferStreamWriter stream(m_ScratchBuffer);
		stream.WriteRaw(PacketType::EntitySpawn);
		SceneSerializer serializer(entity.GetScene());
		std::string jsonData = serializer.SerializeEntityToString(entity);
		stream.WriteString(jsonData);
		SendBufferToAllClients(Buffer(m_ScratchBuffer, stream.GetStreamPosition()));
	}

	void Server::NetworkThreadFunc()
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

	void Server::ConnectionStatusChangedCallback(SteamNetConnectionStatusChangedCallback_t* info) { s_Instance->OnConnectionStatusChanged(info); }

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

	void Server::PollConnectionStateChanges()
	{
		m_Interface->RunCallbacks();
	}

	void Server::OnClientConnected(const ClientInfo& clientInfo)
	{
		// Move logic to PacketType::ClientConnectionRequest handle in OnDataReceived?
		
		std::lock_guard<std::mutex> lock(m_ClientQueueMutex);
		m_ConnectedClientQueue.push(clientInfo);

	}

	void Server::OnClientDisconnected(const ClientInfo& clientInfo)
	{
		m_GameInstance->GetActiveScene()->GetGameMode()->Server_OnClientDisconnected((uint32_t)clientInfo.ID);
	}

	void Server::OnDataReceived(const ClientInfo& clientInfo, const Buffer& buffer)
	{
		BufferStreamReader stream(buffer);
	}

	void Server::OnTick() // Called from main thread
	{
		m_QueueMutex.lock();

		m_ClientQueueMutex.lock();
		while (!m_ConnectedClientQueue.empty())
		{
			ClientInfo& clientInfo = m_ConnectedClientQueue.front();
			{
				BufferStreamWriter stream(m_ScratchBuffer);
				stream.WriteRaw(PacketType::ConnectionAccepted);
				stream.WriteRaw(clientInfo.ID);
				SendBufferToClient(clientInfo.ID, Buffer(m_ScratchBuffer, stream.GetStreamPosition()));
			}

			// PacketType::InitializeScene
			//{
			//	BufferStreamWriter stream(m_ScratchBuffer);
			//	stream.WriteRaw(PacketType::InitializeScene);
			//	SceneSerializer sceneSerializer(m_GameInstance->GetActiveScene());
			//	std::string jsonData = sceneSerializer.Serialize();
			//	stream.WriteString(jsonData);
			//	SendBufferToClient(clientInfo.ID, Buffer(m_ScratchBuffer, stream.GetStreamPosition()));
			//}

			m_GameInstance->GetActiveScene()->GetGameMode()->Server_OnClientConnected((uint32_t)clientInfo.ID);

			m_ConnectedClientQueue.pop();
		}
		m_ClientQueueMutex.unlock();


		while (!m_MessageQueue.empty())
		{
			ISteamNetworkingMessage*& message = m_MessageQueue.front();
			Buffer buffer(message->m_pData, message->m_cbSize);
			BufferStreamReader stream(buffer);
			PacketType packetType;
			stream.ReadRaw(packetType);

			switch (packetType)
			{

			case PacketType::PlayerAction:
				if (m_OnRecvPlayerActionCallbacks.find(message->m_conn) != m_OnRecvPlayerActionCallbacks.end())
				{
					OnRecvPlayerActionCallback& callback = m_OnRecvPlayerActionCallbacks.at(message->m_conn);
					callback(stream);
				}
				else
				{
					PT_CORE_ERROR("OnRecvPlayerActionCallbacks[{}] was not defined!", (uint32_t)message->m_conn);
				}
				break;

			default:
				PT_CORE_ERROR("Invalid packet type ({}).", (uint16_t)packetType);
				// TODO: disconnect client
				break;
			}

			message->Release();
			m_MessageQueue.pop();
		}

		m_QueueMutex.unlock();

		Scene* scene = m_GameInstance->GetActiveScene();

		// Replicate entitites with NetworkComponent
		auto view = scene->GetAllEntitiesWith<NetworkComponent>();
		if (!view.size())
			return;

		BufferStreamWriter stream(m_ScratchBuffer);
		stream.WriteRaw(PacketType::UpdateReplicated);

		for (auto e : view)
		{
			Entity entity(e, scene);

			bool hasSpriteComponent = entity.HasComponent<SpriteComponent>();

			stream.WriteRaw(ReplicatedEntityUpdateInfo{
				entity.GetUUID(),
				hasSpriteComponent
			});

			auto& transform = entity.GetTransform();
			stream.WriteRaw(transform);

			if (hasSpriteComponent)
			{
				auto& sprite = entity.GetSprite();
				stream.WriteRaw(sprite.m_TilePos);
				stream.WriteRaw(sprite.m_MirrorFlip);
			}
		}

		SendBufferToAllClients(Buffer(m_ScratchBuffer, stream.GetStreamPosition()));
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
				//OnDataReceived(itClient->second, Buffer(incomingMessage->m_pData, incomingMessage->m_cbSize));
				m_QueueMutex.lock();
				m_MessageQueue.push(incomingMessage);
				m_QueueMutex.unlock();
			}

			// Release when done
			//incomingMessage->Release();
		}
	}

	void Server::SetClientNick(HSteamNetConnection hConn, const char* nick)
	{
		// Set the connection name, too, which is useful for debugging
		m_Interface->SetConnectionName(hConn, nick);
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

	void Server::KickClient(ClientID clientID)
	{
		m_Interface->CloseConnection(clientID, 0, "Kicked by host", false);
	}

	void Server::OnFatalError(const std::string& message)
	{
		PT_CORE_CRITICAL(message);
		m_Running = false;
	}

}
