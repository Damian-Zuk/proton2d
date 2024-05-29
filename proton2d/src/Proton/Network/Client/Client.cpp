#include "ptpch.h"
#include "Proton/Network/Client/Client.h"
#include "Proton/Network/Common/PacketType.h"
#include "Proton/Network/Common/NetworkManager.h"
#include "Proton/Network/Client/NetClientTransformSyncSystem.h"

#include "Proton/Core/GameInstance.h"
#include "Proton/Assets/SceneSerializer.h"
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
		NetClientTransformSyncSystem::Update(scene, ts);
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
				PROFILE_SCOPE("PacketType::GameStateUpdate");

				uint32_t created, destroyed;
				stream.ReadRaw(created);
				stream.ReadRaw(destroyed);

				// Create missing entities
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

				// Destroy no longer existing entites
				for (uint32_t i = 0; i < destroyed; i++)
				{
					UUID uuid;
					stream.ReadRaw(uuid);
					Entity entity = scene->FindByID(uuid);

					if (!entity)
						continue;
					
					entity.Destroy();
				}
				
				UpdateReplicatedEntities(scene, stream, buffer.Size, true);

				if (m_JustConnected)
				{
					scene->GetGameMode()->Client_OnConnected(m_ServerClientID);
					m_NetworkManager->m_ClientGameStateInitialized = true;
					m_JustConnected = false;
				}

				PT_CORE_INFO("GameStateUpdate: created={}, destroyed={}", created, destroyed);
				m_GameStateInitialized = true;
				break;
			}
			
			////////////////////////////////////////////////////////////////////////////////////////////////////
			
			case PacketType::EntitySpawn:
			{
				PROFILE_SCOPE("PacketType::EntitySpawn");

				if (!m_GameStateInitialized)
					break;

				while (stream.GetStreamPosition() < buffer.Size)
				{
					std::string jsonData;
					stream.ReadString(jsonData);
					json jsonParsed = json::parse(jsonData);

					if (scene->FindByID((UUID)jsonParsed.at("UUID")))
						break;

					SceneSerializer serializer(scene);
					Entity entity = serializer.DeserializeEntity(jsonParsed);
					PT_CORE_INFO("EntitySpawn: {} ({})", entity.GetTag(), entity.GetUUID());
				}
				break;
			
			}
			////////////////////////////////////////////////////////////////////////////////////////////////////
			
			case PacketType::EntityDestroy:
			{
				PROFILE_SCOPE("PacketType::EntityDestroy");

				if (!m_GameStateInitialized)
					break;

				while (stream.GetStreamPosition() < buffer.Size)
				{
					UUID uuid;
					stream.ReadRaw(uuid);
					Entity entity = scene->FindByID(uuid);

					if (!entity)
						continue;

					entity.Destroy();
				}
				break;
			}

			////////////////////////////////////////////////////////////////////////////////////////////////////
			
			case PacketType::UpdateReplicated:
			{
				PROFILE_SCOPE("PacketType::UpdateReplicated");

				if (!m_GameStateInitialized)
					break;

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
		while (stream.GetStreamPosition() < bufferSize - sizeof(uint64) * 2)
		{
			uint64_t entityStreamStart = stream.GetStreamPosition();

			uint64_t entityBufferSize;
			std::bitset<MAX_COMPONENTS> componentBitset;
			UUID entityUUID;

			stream.ReadRaw(entityBufferSize);
			stream.ReadRaw(componentBitset);
			stream.ReadRaw(entityUUID);

			// Find entity
			Entity entity = scene->FindByID(entityUUID);

			if (!entity.IsValid())
			{
				stream.SetStreamPosition(entityStreamStart + entityBufferSize);
				continue;
			}

			if (!entity.HasComponent<NetworkComponent>())
			{
				stream.SetStreamPosition(entityStreamStart + entityBufferSize);
				continue;
			}

			auto& net = entity.GetComponent<NetworkComponent>();
			auto syncMethod = net.SyncMethod == NetTranformSyncMethod::Inherit ? scene->m_NetTranformSyncMethod : net.SyncMethod;
			bool updateTransformImmediately = updateTransformNow || 
				syncMethod == NetTranformSyncMethod::None || syncMethod == NetTranformSyncMethod::Extrapolate;
			bool updateTransformStateTimer = false;

			// TransformComponent
			if (componentBitset.test(ComponentType_Transform))
			{
				auto& position = net.NetTransform.Position;
				auto& rotation = net.NetTransform.Rotation;

				stream.ReadRaw(position);
				stream.ReadRaw(rotation);

				if (updateTransformImmediately)
				{
					auto& transform = entity.GetTransform();
					transform.LocalPosition.x = position.x;
					transform.LocalPosition.y = position.y;
					transform.Rotation = rotation;
				}

				updateTransformStateTimer = true;
			}

			// VelocityComponent
			if (componentBitset.test(ComponentType_Velocity))
			{
				auto& linearVelocity = net.NetTransform.LinearVelocity;
				auto& angularVelocity = net.NetTransform.AngularVelocity;

				stream.ReadRaw(linearVelocity);
				stream.ReadRaw(angularVelocity);

				if (updateTransformImmediately)
				{
					auto& velocity = entity.GetComponent<VelocityComponent>();
					velocity.LinearVelocity = linearVelocity;
					velocity.AngularVelocity = angularVelocity;
				}

				updateTransformStateTimer = true;
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

			if (updateTransformStateTimer)
			{
				net.NetTransform.PacketDelay = net.NetTransform.UpdateTimer.Elapsed();
				net.NetTransform.UpdateTimer.Reset();
			}
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
