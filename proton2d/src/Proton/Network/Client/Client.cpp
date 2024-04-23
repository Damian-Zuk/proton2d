#include "ptpch.h"
#include "Proton/Network/Client/Client.h"
#include "Proton/Network/Common/PacketType.h"
#include "Proton/Network/Common/NetworkManager.h"

#include "Proton/Core/GameInstance.h"
#include "Proton/Assets/SceneSerializer.h"
#include "Proton/Scene/SceneManager.h"
#include "Proton/Scene/Scene.h"
#include "Proton/Scene/Entity.h"
#include "Proton/Scripting/GameModeBase.h"

namespace proton {

	static std::unordered_map<HSteamNetConnection, Client*> s_ConnectionToInstanceMap;
	static std::mutex s_InstanceMapMutex;

	Client::Client(GameInstance* gameInstance)
		: m_GameInstance(gameInstance), m_NetworkManager(gameInstance->m_NetworkManager.get())
	{
		// 1 MB scratch buffer
		m_ScratchBuffer.Allocate(1048576);
	}

	Client::~Client()
	{
		if (m_NetworkThread.joinable())
			m_NetworkThread.join();
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

	void Client::OnDataReceived(ISteamNetworkingMessage* incomingMessage)
	{
		std::lock_guard<std::mutex> lock(m_QueueMutex);
		m_MessageQueue.push(incomingMessage);
	}

	void Client::SendVerifyGameState()
	{
		SceneManager* sceneManager = m_GameInstance->GetSceneManager();
		Scene* scene = sceneManager->GetActiveScene();

		BufferStreamWriter writer(m_ScratchBuffer);
		writer.WriteRaw(PacketType::VerifyGameState);

		auto view = scene->GetAllEntitiesWith<IDComponent, NetworkComponent>();
		for (entt::entity e : view)
		{
			auto& id = view.get<IDComponent>(e);
			writer.WriteRaw(id.ID);
		}

		SendBuffer(Buffer(m_ScratchBuffer, writer.GetStreamPosition()));
	}

	void Client::SendPlayerAction(Client_SendPlayerActionCallback sendFunction)
	{
		BufferStreamWriter stream(m_ScratchBuffer);
		stream.WriteRaw(PacketType::PlayerAction);
		sendFunction(stream);
		SendBuffer(Buffer(m_ScratchBuffer, stream.GetStreamPosition()));
	}

	void Client::MainThread_ProcessMessages()
	{
		std::lock_guard<std::mutex> lock(m_QueueMutex);
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
			
			case PacketType::ConnectionAccepted:
			{
				stream.ReadRaw(m_ServerClientID);
				m_JustConnected = true;
				SendVerifyGameState();
				PT_CORE_TRACE("ConnectionAccepted: client_id={}", m_ServerClientID);
				break;
			}
			
			////////////////////////////////////////////////////////////////////////////////////////////////////
			
			case PacketType::GameStateUpdate:
			{
				uint32_t created, destroyed;
				stream.ReadRaw(created);
				stream.ReadRaw(destroyed);

				for (uint32_t i = 0; i < created; i++)
				{
					std::string jsonData;
					stream.ReadString(jsonData);
					json jsonParsed = json::parse(jsonData);

					if (scene->FindByID((UUID)jsonParsed.at("UUID")))
						continue;

					SceneSerializer serializer(scene);
					Entity entity = serializer.DeserializeEntity(jsonParsed);
				}

				for (uint32_t i = 0; i < destroyed; i++)
				{
					UUID uuid;
					stream.ReadRaw(uuid);
					Entity entity = scene->FindByID(uuid);

					if (!entity)
						continue;
					
					entity.Destroy();
				}

				if (m_JustConnected)
				{
					scene->GetGameMode()->Client_OnConnected(m_ServerClientID);
					m_NetworkManager->m_ClientGameStateInitialized = true;
					m_JustConnected = false;
				}
				break;
			}
			
			////////////////////////////////////////////////////////////////////////////////////////////////////
			
			case PacketType::EntitySpawn:
			{
				while (stream.GetStreamPosition() < buffer.Size)
				{
					std::string jsonData;
					stream.ReadString(jsonData);
					json jsonParsed = json::parse(jsonData);

					if (scene->FindByID((UUID)jsonParsed.at("UUID")))
						break;

					SceneSerializer serializer(scene);
					Entity entity = serializer.DeserializeEntity(jsonParsed);
				}
				break;
			
			}
			////////////////////////////////////////////////////////////////////////////////////////////////////
			
			case PacketType::EntityDestroy:
			{
				UUID uuid;
				stream.ReadRaw(uuid);
				Entity entity = scene->FindByID(uuid);

				if (!entity)
					break;
				
				entity.Destroy();
				break;
			}

			////////////////////////////////////////////////////////////////////////////////////////////////////
			
			case PacketType::UpdateReplicated:
			{
				while (stream.GetStreamPosition() < buffer.Size - sizeof(uint64) * 2)
				{
					uint64_t entityStreamStart = stream.GetStreamPosition();
					uint64_t entityBufferSize;
					UUID entityUUID;

					stream.ReadRaw(entityBufferSize);
					stream.ReadRaw(entityUUID);

					Entity entity = scene->FindByID(entityUUID);

					if (!entity.IsValid())
					{
						stream.SetStreamPosition(entityStreamStart + entityBufferSize);
						continue;
					}

					if (!entity.HasComponent<NetworkComponent>())
					{
						PT_CORE_ERROR("UpdateReplicated: Entity {} missing NetworkComponent!", entityUUID);
						stream.SetStreamPosition(entityStreamStart + entityBufferSize);
						continue;
					}

					// Read replicated data
					auto& net = entity.GetComponent<NetworkComponent>();

					if (net.IsReplicated(ComponentTypeID::Transform))
					{
						auto& transform = entity.GetTransform();
						stream.ReadRaw(transform);
					}

					if (net.IsReplicated(ComponentTypeID::Sprite))
					{
						auto& sprite = entity.GetSprite();
						stream.ReadRaw(sprite.m_TilePos);
						stream.ReadRaw(sprite.m_MirrorFlip);
						sprite.CalculateTextureCoords();
					}
				}
				break;
			}

			////////////////////////////////////////////////////////////////////////////////////////////////////
			default:
				PT_CORE_ERROR("Invalid packet type ({}).", (uint16_t)packetType);
				break;
			}

			// Free message buffer
			message->Release();
			m_MessageQueue.pop();
		}
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////
	////               Network Thread and GameNetworkingSockets Interface Implementation               ////
	///////////////////////////////////////////////////////////////////////////////////////////////////////

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

		// Select instance to use. For now we'll always use the default.
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

			OnDataReceived(incomingMessage);
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
			m_ConnectionStatus = ConnectionStatus::FailedToConnect;
			m_ConnectionDebugMessage = info->m_info.m_szEndDebug;

			// Print an appropriate message
			if (info->m_eOldState == k_ESteamNetworkingConnectionState_Connecting)
			{
				// Note: we could distinguish between a timeout, a rejected connection,
				// or some other transport problem.
				PT_CORE_ERROR("Could not connect to remote host. {}", info->m_info.m_szEndDebug);
			}
			else if (info->m_info.m_eState == k_ESteamNetworkingConnectionState_ProblemDetectedLocally)
			{
				PT_CORE_ERROR("Lost connection with remote host. {}", info->m_info.m_szEndDebug);
			}
			else
			{
				// NOTE: We could check the reason code for a normal disconnection
				PT_CORE_ERROR("Disconnected from host. {}", info->m_info.m_szEndDebug);
			}

			// Clean up the connection.  This is important!
			// The connection is "closed" in the network sense, but
			// it has not been destroyed.  We must close it on our end, too
			// to finish up.  The reason information do not matter in this case,
			// and we cannot linger because it's already closed on the other end,
			// so we just pass 0s.
			m_Interface->CloseConnection(info->m_hConn, 0, nullptr, false);
			m_Connection = k_HSteamNetConnection_Invalid;
			m_ConnectionStatus = ConnectionStatus::Disconnected;
			break;
		}

		case k_ESteamNetworkingConnectionState_Connecting:
			// We will get this callback when we start connecting.
			// We can ignore this.
			break;

		case k_ESteamNetworkingConnectionState_Connected:
			m_ConnectionStatus = ConnectionStatus::Connected;
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
