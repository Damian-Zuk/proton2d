#include "ptpch.h"
#include "Proton/Network/Client.h"
#include "Proton/Network/Packets.h"
#include "Proton/Network/NetworkManager.h"
#include "Proton/Network/NetSyncSystem.h"

#include "Proton/Core/GameInstance.h"
#include "Proton/Scene/SceneSerializer.h"
#include "Proton/Scene/SceneManager.h"
#include "Proton/Scene/Scene.h"
#include "Proton/Scene/Entity.h"
#include "Proton/Scripting/GameModeBase.h"
#include "Proton/Scripting/EntityScript.h"

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
		ProcessMessages();

		Scene* scene = m_GameInstance->GetActiveScene();
		scene->CalculateWorldPositions(true);
		NetSyncSystem::Update(scene, ts);
	}

	void Client::OnDataReceived(ISteamNetworkingMessage* incomingMessage)
	{
		std::lock_guard<std::mutex> lock(m_QueueMutex);
		m_MessageQueue.push(incomingMessage);
	}

	void Client::SendHandshake()
	{
		NetMessageHandshake msg;
		msg.EngineProtocolVersion = PT_NET_PROTOCOL_VERSION;
		msg.GameProtocolVersion = NetworkManager::s_GameProtocolVersion;
		BufferStreamWriter stream(m_ScratchBuffer);
		stream.WriteRaw(msg);
		SendBuffer(stream.GetBuffer());
	}

	void Client::SendPlayerAction(StreamWriterDelegate sendFunction)
	{
		BufferStreamWriter stream(m_ScratchBuffer);
		stream.WriteRaw(PacketType::PlayerAction);
		sendFunction(stream);
		SendBuffer(Buffer(m_ScratchBuffer, stream.GetStreamPosition()));
	}

	void Client::ProcessMessages()
	{
		PROFILE_FUNCTION();

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
			stream.SetStreamPosition(0);

			switch (packetType)
			{
			////////////////////////////////////////////////////////////////////////////////////////////////////

			case PacketType::HandshakeReply:
			{
				NetMessageHandshakeReply msg;
				stream.ReadRaw(msg);

				m_LocalClientID = msg.ClientID;
				m_NetworkManager->m_LocalClientID = m_LocalClientID;
				m_ConnectionStatus = ConnectionStatus::Connected;
				PT_CORE_INFO("Successfully connected to server");
				// GameModeBase::Client_OnConnected called after first replication update

				break;
			}

			////////////////////////////////////////////////////////////////////////////////////////////////////
			
			case PacketType::EntitySpawn:
			{
				PROFILE_SCOPE("PacketType::EntitySpawn");

				NetMessageSpawn msgx;
				stream.ReadRaw(msgx);

				for (uint32_t i = 0; i < msgx.EntityCount; i++)
				{
					NetMessageSpawn::PayloadItem item;
					stream.ReadString(item.EntityJsonData);

					json jsonParsed = json::parse(item.EntityJsonData);

					if (scene->FindByID((UUID)jsonParsed.at("UUID")))
						break;

					SceneSerializer serializer(scene);
					Entity entity = serializer.DeserializeEntity(jsonParsed);
				}
				break;
			}

			////////////////////////////////////////////////////////////////////////////////////////////////////
			
			case PacketType::EntityDespawn:
			{
				PROFILE_SCOPE("PacketType::EntityDespawn");

				NetMessageDespawn msg;
				stream.ReadRaw(msg);

				for (uint32_t i = 0; i < msg.EntityCount; i++)
				{
					NetMessageDespawn::PayloadItem item;
					stream.ReadRaw(item);

					Entity entity = scene->FindByID(item.EntityUUID);
					if (!entity)
						continue;

					entity.Destroy();
				}
				break;
			}

			////////////////////////////////////////////////////////////////////////////////////////////////////
			
			case PacketType::EntityReplicate:
			{
				PROFILE_SCOPE("PacketType::EntityReplicate");

				UpdateReplicatedEntities(scene, stream, buffer.Size);
				
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

	void Client::UpdateReplicatedEntities(Scene* scene, BufferStreamReader& stream, uint64_t bufferSize, bool updateTransformNow)
	{
		NetMessageReplicate header;
		stream.ReadRaw(header);

		for (uint32_t i = 0; i < header.EntityCount; i++)
		{
			uint64_t entityStreamStart = stream.GetStreamPosition();

			NetMessageReplicate::PayloadItem item;
			stream.ReadRaw(item);

			auto& componentBitset = item.ComponentBitset;

			// Find entity
			Entity entity = scene->FindByID(item.EntityUUID);

			if (!entity.IsValid())
			{
				stream.SetStreamPosition(entityStreamStart + item.PayloadSize);
				continue;
			}

			if (!entity.HasComponent<NetworkComponent>())
			{
				stream.SetStreamPosition(entityStreamStart + item.PayloadSize);
				continue;
			}

			auto& net = entity.GetComponent<NetworkComponent>();
			auto& transform = entity.GetTransform();
			auto& velocity = entity.GetComponent<VelocityComponent>();

			// TransformComponent
			if (componentBitset.test(ComponentType_Transform))
			{
				net.PreviousTransform.Position = net.CurrentTransform.Position;
				net.PreviousTransform.Rotation = net.CurrentTransform.Rotation;

				stream.ReadRaw(net.CurrentTransform.Position);
				stream.ReadRaw(net.CurrentTransform.Rotation);

				if (net.SyncParams.SyncMethod == NetSyncMethod::None)
				{
					transform.LocalPosition.x = net.CurrentTransform.Position.x;
					transform.LocalPosition.y = net.CurrentTransform.Position.y;
					transform.Rotation = net.CurrentTransform.Rotation;
				}

				// Update packet timer
				net.SyncState.PacketDelay = net.SyncState.PacketTimer.Elapsed();
				net.SyncState.PacketTimer.Reset();
				net.SyncState.NewPacket = true;
			}

			// VelocityComponent
			if (componentBitset.test(ComponentType_Velocity))
			{
				net.PreviousTransform.LinearVelocity = net.CurrentTransform.LinearVelocity;
				net.PreviousTransform.AngularVelocity = net.CurrentTransform.AngularVelocity;

				stream.ReadRaw(net.CurrentTransform.LinearVelocity);
				stream.ReadRaw(net.CurrentTransform.AngularVelocity);

				if (net.SyncParams.SyncMethod == NetSyncMethod::None)
				{
					velocity.LinearVelocity = net.CurrentTransform.LinearVelocity;
					velocity.AngularVelocity = net.CurrentTransform.AngularVelocity;
				}
			}

			// ScriptComponent
			if (componentBitset.test(ComponentType_Script))
			{
				auto& scriptComponent = entity.GetComponent<ScriptComponent>();
				uint32_t scriptCount;
				stream.ReadRaw(scriptCount);

				// For each script
				for (uint32_t i = 0; i < scriptCount; i++)
				{
					uint32_t fieldCount;
					uint32_t scriptIndex;

					stream.ReadRaw(fieldCount);
					stream.ReadRaw(scriptIndex);

					auto& script = net.ReplicatedScripts.at(scriptIndex);

					// For each script field
					for (uint32_t j = 0; j < fieldCount; j++)
					{
						uint32_t fieldIndex;
						stream.ReadRaw(fieldIndex);

						auto& field = script.ReplicatedFields.at(fieldIndex);
						stream.ReadData((char*)field.Data, field.Size);

						// Call notify function
						if (field.NotifyFunction)
							field.NotifyFunction(&entity);
					}
				}
			}
		}

		if (!m_NetworkManager->m_ClientGameStateInitialized)
		{
			// First replication update
			m_GameInstance->GetActiveScene()->GetGameMode()->Client_OnConnected(m_LocalClientID);
			m_NetworkManager->m_ClientGameStateInitialized = true;
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

			// Clean up the connection.  This is important!
			// The connection is "closed" in the network sense, but
			// it has not been destroyed.  We must close it on our end, too
			// to finish up.  The reason information do not matter in this case,
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
