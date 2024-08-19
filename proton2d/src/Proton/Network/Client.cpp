#include "ptpch.h"
#include "Proton/Network/Client.h"
#include "Proton/Network/Messages.h"
#include "Proton/Network/NetworkManager.h"
#include "Proton/Network/NetReplicator.h"
#include "Proton/Network/NetTransformSystem.h"

#include "Proton/Core/GameInstance.h"
#include "Proton/Scene/SceneSerializer.h"
#include "Proton/Scene/SceneManager.h"
#include "Proton/Scene/Scene.h"
#include "Proton/Scene/Entity.h"
#include "Proton/Scripting/GameModeBase.h"

namespace proton {

	static std::unordered_map<HSteamNetConnection, Client*> s_ConnectionToInstanceMap;
	static std::mutex s_InstanceMapMutex;

	Client::Client(GameInstance* gameInstance)
		: m_GameInstance(gameInstance), m_NetworkManager(gameInstance->m_NetworkManager.get()),
		m_NetReplicator(MakeUnique<NetReplicator>(this)),
		m_NetTransformSystem(MakeUnique<NetTransformSystem>(this))
	{
		// 128KB preallocated buffer for writting network messages
		// Will be automaticly resized by NetworkStreamWriter when needed
		m_ScratchBuffer.Allocate(131072);
	}

	Client::~Client()
	{
		if (m_NetworkThread.joinable())
			m_NetworkThread.join();

		m_ScratchBuffer.Release();
	}

	void Client::ConnectToServer(const std::string& serverAddress)
	{
		if (m_Running)
			return;

		if (m_NetworkThread.joinable())
			m_NetworkThread.join();

		m_ServerAddress = serverAddress;
		m_NetworkThread = std::thread([this]() { NetworkThreadFunction(); });
	}

	void Client::Disconnect()
	{
		PT_CORE_INFO("Disconnecting...");
		m_Running = false;

		if (m_NetworkThread.joinable())
			m_NetworkThread.join();
	}

	void Client::Shutdown()
	{
		m_Running = false;
	}

	void Client::MainThread_OnUpdate(float ts)
	{
		PROFILE_FUNCTION();

		std::lock_guard<std::mutex> lock(m_QueueMutex);
		Scene* scene = m_GameInstance->GetActiveScene();

		while (!m_MessageQueue.empty())
		{
			ISteamNetworkingMessage* message = m_MessageQueue.front();
			OnNetworkMessage(message);
			message->Release();
			m_MessageQueue.pop();
		}
		
		scene->CalculateWorldPositions(true);

		if (m_NetworkManager->m_ClientGameStateInitialized)
		{
			m_NetTransformSystem->OnUpdate(scene, ts);
		}
	}

	void Client::SendHandshake()
	{
		NetMessageHandshake header;
		header.EngineProtocolVersion = PROTON_NET_PROTOCOL_VERSION;
		header.GameProtocolVersion = NetworkManager::s_GameProtocolVersion;
		NetworkStreamWriter stream(m_ScratchBuffer);
		stream.WriteRaw(header);
		SendBuffer(stream.GetBuffer());
	}

	void Client::SendPlayerAction(NetworkStreamWriterDelegate sendFunction)
	{
		NetworkStreamWriter stream(m_ScratchBuffer);
		stream.WriteRaw(MessageType::PlayerAction);
		sendFunction(stream);
		SendBuffer(stream.GetBuffer());
	}

	void Client::OnNetworkMessage(ISteamNetworkingMessage* message)
	{
		Buffer buffer(message->m_pData, message->m_cbSize);
		NetworkStreamReader stream(buffer);

		MessageType packetType;
		stream.ReadRaw(packetType);
		stream.SetStreamPosition(0);

		switch (packetType)
		{
			///////////////////////////////////////////////////////////////////////////////////////
		case MessageType::HandshakeReply:
		{
			NetMessageHandshakeReply reply;
			stream.ReadRaw(reply);

			m_LocalClientID = reply.ClientID;
			m_NetworkManager->m_LocalClientID = m_LocalClientID;
			m_ConnectionStatus = ConnectionStatus::Connected;
			PT_CORE_INFO("Connected to server");
			// GameModeBase::Client_OnConnected is called after first replication update
			break;
		}
		///////////////////////////////////////////////////////////////////////////////////////
		case MessageType::EntityReplicate:
		{
			m_NetReplicator->Client_ProcessReplicationMessage(stream);

			if (!m_NetworkManager->m_ClientGameStateInitialized)
			{
				// First replication update
				GameModeBase* gameMode = m_GameInstance->GetActiveScene()->GetGameMode();
				gameMode->Client_OnConnected(m_LocalClientID);
				m_NetworkManager->m_ClientGameStateInitialized = true;
			}
			break;
		}
		///////////////////////////////////////////////////////////////////////////////////////
		case MessageType::EntitySpawn:
		{
			m_NetReplicator->Client_OnEntitySpawnMessage(stream);
			break;
		}
		///////////////////////////////////////////////////////////////////////////////////////
		case MessageType::EntityDespawn:
		{
			m_NetReplicator->Client_OnEntityDespawnMessage(stream);
			break;
		}
		///////////////////////////////////////////////////////////////////////////////////////
		default:
			PT_CORE_ERROR("Invalid packet type ({}).", (uint16_t)packetType);
			break;
		}
	}

	void Client::NetworkThreadFunction()
	{
		// Reset connection status
		m_ConnectionStatus = ConnectionStatus::Connecting;

		SteamDatagramErrMsg errMsg;
		if (!GameNetworkingSockets_Init(nullptr, errMsg))
		{
			m_ConnectionDebugMessage = "Could not initialize GameNetworkingSockets: {}";
			m_ConnectionStatus = ConnectionStatus::FailedToConnect;
			return;
		}

		m_Interface = SteamNetworkingSockets();

		// Start connecting
		SteamNetworkingIPAddr address;
		if (!address.ParseString(m_ServerAddress.c_str()))
		{
			OnFatalError(fmt::format("Invalid IP address - could not parse {}", m_ServerAddress));
			m_ConnectionDebugMessage = "Invalid IP address";
			m_ConnectionStatus = ConnectionStatus::FailedToConnect;
			return;
		}

		SteamNetworkingConfigValue_t options;
		options.SetPtr(k_ESteamNetworkingConfig_Callback_ConnectionStatusChanged, (void*)ConnectionStatusChangedCallback);
		
		m_Connection = m_Interface->ConnectByIPAddress(address, 1, &options);
		if (m_Connection == k_HSteamNetConnection_Invalid)
		{
			m_ConnectionDebugMessage = "Failed to create connection";
			m_ConnectionStatus = ConnectionStatus::FailedToConnect;
			return;
		}

		s_InstanceMapMutex.lock();
		s_ConnectionToInstanceMap[m_Connection] = this;
		s_InstanceMapMutex.unlock();

		PT_CORE_TRACE("Starting client network thread");
		m_Running = true;
		while (m_Running)
		{
			PollIncomingMessages();
			PollConnectionStateChanges();
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
		}

		m_Interface->CloseConnection(m_Connection, 0, nullptr, false);
		m_ConnectionStatus = ConnectionStatus::Disconnected;

		m_NetworkThreadFinished = true;
	}

	void Client::SendBuffer(Buffer buffer, bool reliable)
	{
		EResult result = m_Interface->SendMessageToConnection(m_Connection, buffer.Data, (uint32_t)buffer.Size, reliable ? k_nSteamNetworkingSend_Reliable : k_nSteamNetworkingSend_Unreliable, nullptr);
		// handle result?
	}

	void Client::SendString(const std::string& string, bool reliable)
	{
		SendBuffer(Buffer(string.data(), string.size()), reliable);
	}

	void Client::PollIncomingMessages()
	{
		// Process all messages
		while (m_Running)
		{
			ISteamNetworkingMessage* incomingMessage = nullptr;
			int messageCount = m_Interface->ReceiveMessagesOnConnection(m_Connection, &incomingMessage, 1);
			if (messageCount == 0)
				break;

			if (messageCount < 0)
			{
				// messageCount < 0 means critical error?
				m_Running = false;
				return;
			}

			std::lock_guard<std::mutex> lock(m_QueueMutex);
			m_MessageQueue.push(incomingMessage);
		}
	}

	void Client::PollConnectionStateChanges()
	{
		m_Interface->RunCallbacks();
	}

	void Client::ConnectionStatusChangedCallback(SteamNetConnectionStatusChangedCallback_t* info) 
	{
		std::lock_guard<std::mutex> lock(s_InstanceMapMutex);
		s_ConnectionToInstanceMap.at(info->m_hConn)->OnConnectionStatusChanged(info);
	}

	void Client::OnConnectionStatusChanged(SteamNetConnectionStatusChangedCallback_t* info)
	{
		//assert(pInfo->m_hConn == m_hConnection || m_hConnection == k_HSteamNetConnection_Invalid);

		// Handle connection state
		switch (info->m_info.m_eState)
		{
		case k_ESteamNetworkingConnectionState_None:
			// NOTE: We will get callbacks here when we destroy connections. You can ignore these.
			break;

		case k_ESteamNetworkingConnectionState_ClosedByPeer:
		case k_ESteamNetworkingConnectionState_ProblemDetectedLocally:
		{
			m_Running = false;
			m_ConnectionStatus = ConnectionStatus::Disconnected;
			m_ConnectionDebugMessage = info->m_info.m_szEndDebug;

			// Print an appropriate message
			if (info->m_eOldState == k_ESteamNetworkingConnectionState_Connecting)
			{
				// Note: we could distinguish between a timeout, a rejected connection,
				// or some other transport problem. (info->m_info.m_eEndReason)
				PT_CORE_ERROR("Could not connect to remote host. {}", m_ConnectionDebugMessage);
				m_ConnectionStatus = ConnectionStatus::FailedToConnect;
			}
			else if (info->m_info.m_eState == k_ESteamNetworkingConnectionState_ProblemDetectedLocally)
			{
				PT_CORE_ERROR("Lost connection with remote host. {}", m_ConnectionDebugMessage);
			}
			else
			{
				// NOTE: We could check the reason code for a normal disconnection
				PT_CORE_ERROR("Disconnected from host. {}", m_ConnectionDebugMessage);
			}

			// Clean up the connection. This is important!
			// The connection is "closed" in the network sense, but
			// it has not been destroyed. We must close it on our end, too
			// to finish up. The reason information do not matter in this case,
			// and we cannot linger because it's already closed on the other end,
			// so we just pass 0s.
			m_Interface->CloseConnection(info->m_hConn, 0, nullptr, false);
			m_Connection = k_HSteamNetConnection_Invalid;
			break;
		}

		case k_ESteamNetworkingConnectionState_Connecting:
			// We will get this callback when we start connecting.
			// We can ignore this.
			break;

		case k_ESteamNetworkingConnectionState_Connected:
			SendHandshake();
			break;

		default:
			break;
		}
	}

	void Client::OnFatalError(const std::string& message)
	{
		PT_CORE_ERROR(message);
		m_Running = false;
	}

}
