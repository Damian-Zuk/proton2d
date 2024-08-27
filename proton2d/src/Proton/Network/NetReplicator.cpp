#include "ptpch.h"
#include "Proton/Network/NetReplicator.h"
#include "Proton/Network/Server.h"
#include "Proton/Network/Client.h"
#include "Proton/Network/Messages.h"
#include "Proton/Network/NetStatistics.h"

#include "Proton/Core/GameInstance.h"
#include "Proton/Scene/Scene.h"
#include "Proton/Scene/Entity.h"
#include "Proton/Scene/SceneSerializer.h"
#include "Proton/Scene/SceneManager.h"
#include "Proton/Scene/PrefabManager.h"

#include <Crc32.h>

#ifdef _MSC_VER
	#include <intrin.h>
	static inline int CountSetBits(int n) {
		return __popcnt(n);
	}
#elif defined(__GNUC__)
	static inline int CountSetBits(int n) {
		return __builtin_popcount(n);
	}
#else
	// Fallback implementation (Hamming weights: Parallel reduction)
	// https://hpc.ac.upc.edu/PDFs/dir19/file004398.pdf (Fig. 5)
	static inline uint32_t CountSetBits(uint32_t n) {
		n = (n & 0x55555555u) + ((n >> 1) & 0x55555555u);
		n = (n & 0x33333333u) + ((n >> 2) & 0x33333333u);
		n = (n & 0x0f0f0f0fu) + ((n >> 4) & 0x0f0f0f0fu);
		n = (n & 0x00ff00ffu) + ((n >> 8) & 0x00ff00ffu);
		n = (n & 0x0000ffffu) + ((n >> 16) & 0x0000ffffu);
		return n;
	}
#endif

namespace proton {

	NetReplicator::NetReplicator(Client* client)
		: m_Client(client)
	{
	}
	NetReplicator::NetReplicator(Server* server)
		: m_Server(server)
	{
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	////                                                 Client Replicator                                              ////
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	void NetReplicator::Client_ProcessReplicationMessage(NetworkStreamReader& stream)
	{
		PROFILE_FUNCTION();

		using ReplicateComponents = NetTransform::ReplicateComponents;
		Scene* scene = m_Client->m_GameInstance->GetActiveScene();

		MessageEntityReplicate header;
		stream.ReadRaw(header); // if fails, then header.EntityCount is 0

		// Loop for each entity stored in message payload
		for (uint32_t entityIndex = 0; entityIndex < header.EntityCount; entityIndex++)
		{
			// ------------------------------------- Read entity payload header -------------------------------------
			MessageEntityReplicate::PayloadItem item;

			if (!stream.ReadRaw(item))
			{
				PT_CORE_ERROR("Failed to read payload item header: Stream out of memory. (entity_index={}, entity_count={})", entityIndex, header.EntityCount);
				return;
			}

			// ---------------------- Validate the message and find replicated entity in scene ----------------------
			// Calculate remaining message buffer size
			const uint64_t entityPayloadStart = stream.GetStreamPosition();
			const uint64_t remainingBufferSize = stream.GetRemainingBufferSize();

			// Log payload item header content
			#define DUMP_PAYLOAD_ITEM_HEADER() \
				_PT_CORE_ERROR("Payload item header: (entity_index={}, entity_count={}, entity_uuid={}, transform_flags={}, script_count={}, payload_size={}", \
					entityIndex, header.EntityCount, item.EntityUUID, (int)item.TransformFlags, item.ScriptCount, item.PayloadSize)

			// Set stream to the next entity in message payload
			#define STREAM_NEXT_PAYLOAD_ITEM() \
				stream.SetStreamPosition(entityPayloadStart + item.PayloadSize);

			// Validate if payload size stored in item header does not exceed message buffer size
			if (item.PayloadSize > remainingBufferSize)
			{
				PT_THROW_ERROR("Failed to read payload: item.PayloadSize exceeds remaining message buffer size ({} bytes)", remainingBufferSize);
				DUMP_PAYLOAD_ITEM_HEADER();
				return;
			}

			// Transform components replication flags (each set bit corresponds to one float value in transform payload)
			const auto& flags = item.TransformFlags; 
			
			// Calculate NetTransform payload size that will be read from stream based on the flags from item header
			const uint64_t netTransformPayloadSize = CountSetBits((int)flags) * sizeof(float);

			// Validate if transform values does not exceed remaining message buffer size
			if (netTransformPayloadSize > remainingBufferSize)
			{
				PT_THROW_ERROR("Failed to read payload: item.TransformFlags does not match remaining message buffer size ({} bytes)", remainingBufferSize);
				DUMP_PAYLOAD_ITEM_HEADER();
				return;
			}

			// Find entity by UUID
			Entity entity = scene->FindByID(item.EntityUUID);
			if (!entity.IsValid())
			{
				PT_THROW_ERROR("Failed to find Entity {} (entity_index={}, entity_count={})", item.EntityUUID, entityIndex, header.EntityCount);
				STREAM_NEXT_PAYLOAD_ITEM();
				continue;
			}

			// Make sure entity has NetworkComponent
			if (!entity.HasComponent<NetworkComponent>())
			{
				PT_THROW_ERROR("Entity {} is missing NetworkComponent (entity_index={}, entity_count={})", item.EntityUUID, entityIndex, header.EntityCount);
				STREAM_NEXT_PAYLOAD_ITEM();
				continue;
			}
			auto& net = entity.GetComponent<NetworkComponent>();

			// ------------------------------------ TransformComponent Replication ------------------------------------
			auto& netTransform = net.NetTransform;
			auto& lastTransform = netTransform.LastAuthoritativeTransform;
			auto& prevTransform = netTransform.PrevAuthoritativeTransform;
		
			net.NetTransform.ReplicationFlags = flags;
			
			// If any transform components is replicated, reset NetTransform::SyncState
			if (flags != ReplicateComponents::None)
			{
				prevTransform = lastTransform;
				netTransform.ServerSequenceNumber = item.TransformSequenceNumber;
				netTransform.ReplicationTimer = 0.0f;
				netTransform.InterpolationTimer = 0.0f;
			}
			
			// Read transform values from message payload
			if (EnumHasAllFlags(flags, ReplicateComponents::All))
			{
				stream.ReadRaw(lastTransform);
			}
			else
			{
				if (EnumHasAllFlags(flags, ReplicateComponents::Position))
				{
					stream.ReadRaw(lastTransform.Position);
				}
				else if (EnumHasAnyFlags(flags, ReplicateComponents::Position))
				{
					if (EnumHasAnyFlags(flags, ReplicateComponents::PositionX))
						stream.ReadRaw(lastTransform.Position.x);
					if (EnumHasAnyFlags(flags, ReplicateComponents::PositionY))
						stream.ReadRaw(lastTransform.Position.y);
				}

				if (EnumHasAllFlags(flags, ReplicateComponents::Scale))
				{
					stream.ReadRaw(lastTransform.Scale);
				}
				else if (EnumHasAnyFlags(flags, ReplicateComponents::Scale))
				{
					if (EnumHasAnyFlags(flags, ReplicateComponents::ScaleX))
						stream.ReadRaw(lastTransform.Scale.x);
					if (EnumHasAnyFlags(flags, ReplicateComponents::ScaleY))
						stream.ReadRaw(lastTransform.Scale.y);
				}

				if (EnumHasAnyFlags(flags, ReplicateComponents::Rotation))
				{
					stream.ReadRaw(lastTransform.Rotation);
				}
			}

			// ------------------------------------ ScriptComponent Replication ------------------------------------
			for (uint32_t i = 0; i < item.ScriptCount; i++)
			{
				uint32_t scriptIndex;
				uint32_t fieldCount;

				if (!stream.ReadRaw(scriptIndex))
				{
					PT_THROW_ERROR("Failed to read script index: Stream out of memory (i={})", i);
					DUMP_PAYLOAD_ITEM_HEADER();
					return;
				}

				if (!stream.ReadRaw(fieldCount))
				{
					PT_THROW_ERROR("Failed to read script field count: Stream out of memory (i={})", i);
					DUMP_PAYLOAD_ITEM_HEADER();
					return;
				}

				if (scriptIndex >= net.ReplicatedScripts.size())
				{
					PT_THROW_ERROR("Script index out of bounds: net.ReplicatedScripts[{}] but size is {}", scriptIndex, net.ReplicatedScripts.size());
					DUMP_PAYLOAD_ITEM_HEADER(); STREAM_NEXT_PAYLOAD_ITEM();
					break;
				}
				auto& script = net.ReplicatedScripts[scriptIndex];

				// Loop for each script field in payload
				bool success = true;
				for (uint32_t j = 0; j < fieldCount; j++)
				{
					uint32_t fieldIndex;
					if (!stream.ReadRaw(fieldIndex))
					{
						PT_THROW_ERROR("Failed to read script field index: Stream out of memory (i={}, j={})", i, j);
						DUMP_PAYLOAD_ITEM_HEADER();
						return;
					}

					if (fieldIndex >= script.ReplicatedFields.size())
					{
						PT_THROW_ERROR("Script field index out of bounds (net.ReplicatedScripts[{}], script.ReplicatedFields[{}])", scriptIndex, fieldIndex);
						DUMP_PAYLOAD_ITEM_HEADER(); STREAM_NEXT_PAYLOAD_ITEM();
						success = false;
						break;
					}

					auto& field = script.ReplicatedFields[fieldIndex];
					
					// Read field value from the replication message
					if (!stream.ReadData((char*)field.Data, field.Size))
					{
						PT_THROW_ERROR("Failed to read script field data: Stream buffer of memory (i={}, j={})", i, j);
						DUMP_PAYLOAD_ITEM_HEADER();
						return;
					}

					// Call notify function (if defined)
					if (field.NotifyFunction != nullptr)
						field.NotifyFunction();
				}

				if (!success)
					break;
			}
			net.WasReplicated = true;
		}
	}

	void NetReplicator::Client_OnEntitySpawnMessage(NetworkStreamReader& stream)
	{
		PROFILE_FUNCTION();
		Scene* scene = m_Client->m_GameInstance->GetActiveScene();

		MessageEntitySpawn header;
		stream.ReadRaw(header);

		for (uint32_t i = 0; i < header.EntityCount; i++)
		{
			MessageEntitySpawn::PayloadItem item;
			uint64_t streamStart = stream.GetStreamPosition();
			stream.ReadRaw(item);

			if (scene->FindByID(item.EntityUUID))
			{
				PT_CORE_ERROR("Entity {} already exists", item.EntityUUID);
				stream.SetStreamPosition(streamStart + item.PayloadSize);
				continue;
			}

			Entity entity;

			if (item.PrefabUUID)
			{
				entity = PrefabManager::Spawn(scene, item.PrefabUUID, item.EntityUUID);
			}
			else
			{
				std::string jsonEntity;
				stream.ReadString(jsonEntity);
				SceneSerializer serializer(scene);
				entity = serializer.DeserializeEntity(json::parse(jsonEntity));
			}

			if (item.ParentUUID)
			{
				if (Entity parent = scene->FindByID(item.ParentUUID))
				{
					parent.AddChildEntity(entity);
				}
				else
				{
					PT_CORE_ERROR("Could not find parent entity {} for a spawned entity {}", item.ParentUUID, item.EntityUUID);
				}
			}
			
			auto& net = entity.GetComponent<NetworkComponent>();
			bool hasRigidbodySimulated = net.SimulateOnClient && entity.HasComponent<RigidbodyComponent>();
			net.NetTransform.LastAuthoritativeTransform.Position = item.Position;
			if (hasRigidbodySimulated)
				entity.SetWorldPosition(item.Position);
			else
				entity.SetLocalPosition(item.Position);
		}
	}

	void NetReplicator::Client_OnEntityDespawnMessage(NetworkStreamReader& stream)
	{
		PROFILE_FUNCTION();
		Scene* scene = m_Client->m_GameInstance->GetActiveScene();

		MessageEntityDespawn header;
		stream.ReadRaw(header);

		for (uint32_t i = 0; i < header.EntityCount; i++)
		{
			MessageEntityDespawn::PayloadItem item;
			stream.ReadRaw(item);

			Entity entity = scene->FindByID(item.EntityUUID);
			if (!entity)
				continue;

			entity.Destroy();
		}
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	////                                               Server Replicator                                                ////
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	void NetReplicator::Server_OnUpdate()
	{
		PROFILE_FUNCTION();

		SceneManager* sceneManager = m_Server->m_GameInstance->GetSceneManager();

		for (const auto& kv : sceneManager->m_Scenes)
		{
			Scene* scene = kv.second.get();
			if (scene->IsSimulated())
			{
				Server_ProcessSpawnedEntityQueue(scene);
				Server_ProcessDespawnedEntityQueue(scene);
			}
		}

		for (auto& [clientID, clientInfo] : m_Server->m_ConnectedClients)
		{
			if (clientInfo.Status == ConnectionStatus::Connected)
				Server_SendReplicationMessage(clientID);
		}
	}

	void NetReplicator::Server_OnClientConnected(ClientID clientID)
	{
		PROFILE_FUNCTION();
		Scene* scene = m_Server->m_GameInstance->GetActiveScene();
		Server_SendAllSpawnedEntities(scene, clientID);
		Server_SendAllDespawnedEntities(scene, clientID);
		Server_SendReplicationMessage(clientID, true);
	}

	void NetReplicator::Server_AddSpawnedEntity(Scene* scene, UUID entityUUID)
	{
		auto& sceneData = m_SceneData[scene];
		sceneData.SpawnedEntityQueue.push(entityUUID);
	}

	void NetReplicator::Server_AddDespawnedEntity(Scene* scene, UUID entityUUID)
	{
		auto& sceneData = m_SceneData[scene];
		sceneData.DespawnedEntityQueue.push(entityUUID);
	}

	static void Server_WriteSpawnedEntityPayloadItem(NetworkStreamWriter& stream, Entity entity)
	{
		MessageEntitySpawn::PayloadItem item;
		uint64_t streamStart = stream.GetStreamPosition();
		stream.SkipBytes(sizeof(item));

		auto& net = entity.GetComponent<NetworkComponent>();

		item.EntityUUID = entity.GetUUID();
		bool hasRigidbodySimulated = net.SimulateOnClient && entity.HasComponent<RigidbodyComponent>();
		auto& transform = entity.GetTransform();
		auto& position = hasRigidbodySimulated ? transform.WorldPosition : transform.LocalPosition;
		item.Position = position;

		if (Entity parent = entity.GetParent())
		{
			item.ParentUUID = parent.GetUUID();
		}

		if (entity.HasComponent<PrefabComponent>())
		{
			auto& pc = entity.GetComponent<PrefabComponent>();
			item.PrefabUUID = pc.PrefabUUID;
		}
		else
		{
			SceneSerializer serializer(entity.GetScene());
			std::string entityJson = serializer.SerializeEntityToString(entity);
			stream.WriteString(entityJson);
		}

		item.PayloadSize = stream.GetStreamPosition() - streamStart;

		stream.WriteRawAt(streamStart, item);
	}

	void NetReplicator::Server_ProcessSpawnedEntityQueue(Scene* scene)
	{
		PROFILE_FUNCTION();
		auto& sceneData = m_SceneData[scene];
		if (sceneData.SpawnedEntityQueue.empty())
			return;

		MessageEntitySpawn header;
		NetworkStreamWriter stream(m_Server->m_ScratchBuffer);
		stream.SkipBytes(sizeof(header));

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
			
			Server_WriteSpawnedEntityPayloadItem(stream, entity);

			spawned++;
			sceneData.SpawnedEntityQueue.pop();
			sceneData.SpawnedAll.push_back(uuid);
		}

		if (spawned > 0)
		{
			header.EntityCount = spawned;
			stream.WriteRawAt(0, header);
			m_Server->SendBufferToAllClients(stream.GetBuffer());
		}
	}

	void NetReplicator::Server_ProcessDespawnedEntityQueue(Scene* scene)
	{
		PROFILE_FUNCTION();
		auto& sceneData = m_SceneData[scene];
		if (sceneData.DespawnedEntityQueue.empty())
			return;

		MessageEntityDespawn header;
		NetworkStreamWriter stream(m_Server->m_ScratchBuffer);
		stream.SkipBytes(sizeof(header));

		uint32_t despawned = 0;
		while (!sceneData.DespawnedEntityQueue.empty())
		{
			UUID uuid = sceneData.DespawnedEntityQueue.front();

			stream.WriteRaw(uuid);
			despawned++;

			auto& spawnedAll = sceneData.SpawnedAll;
			auto spawnedIt = std::find(spawnedAll.begin(), spawnedAll.end(), uuid);
			if (spawnedIt != spawnedAll.end())
				spawnedAll.erase(spawnedIt);
			else
				sceneData.DespawnedAll.push_back(uuid);
			
			sceneData.DespawnedEntityQueue.pop();
		}

		if (despawned > 0)
		{
			header.EntityCount = despawned;
			stream.WriteRawAt(0, header);
			m_Server->SendBufferToAllClients(stream.GetBuffer());
		}
	}

	void NetReplicator::Server_SendAllSpawnedEntities(Scene* scene, ClientID clientID)
	{
		PROFILE_FUNCTION();
		const auto& sceneData = m_SceneData[scene];
		const auto& spawnedAll = sceneData.SpawnedAll;
		if (spawnedAll.size() > 0)
		{
			NetworkStreamWriter stream(m_Server->m_ScratchBuffer);

			MessageEntitySpawn msg;
			msg.EntityCount = (uint32_t)spawnedAll.size();
			stream.WriteRaw(msg);

			for (UUID uuid : spawnedAll)
			{
				Entity entity = scene->FindByID(uuid);
				Server_WriteSpawnedEntityPayloadItem(stream, entity);
			}

			m_Server->SendBufferToClient(clientID, stream.GetBuffer());
		}
	}

	void NetReplicator::Server_SendAllDespawnedEntities(Scene* scene, ClientID clientID)
	{
		PROFILE_FUNCTION();
		const auto& sceneData = m_SceneData[scene];
		const auto& despawnedAll = sceneData.DespawnedAll;
		if (despawnedAll.size() > 0)
		{
			NetworkStreamWriter stream(m_Server->m_ScratchBuffer);

			MessageEntityDespawn msg;
			msg.EntityCount = (uint32_t)despawnedAll.size();
			stream.WriteRaw(msg);

			for (UUID uuid : despawnedAll)
			{
				MessageEntityDespawn::PayloadItem item;
				item.EntityUUID = uuid;
				stream.WriteRaw(item);
			}

			m_Server->SendBufferToClient(clientID, stream.GetBuffer());
		}
	}

	void NetReplicator::Server_SendReplicationMessage(ClientID clientID, bool forceReplication)
	{
		PROFILE_FUNCTION();

		Entity clientEntity = m_Server->GetClientEntity(clientID);
		Scene* scene = clientEntity ? clientEntity.m_Scene : m_Server->m_GameInstance->GetActiveScene();
		
		// Skip if scene is not simulated
		if (scene->GetSceneState() == SceneState::Stop)
			return;

		// There is no need to replicate if no clients are connected
		if (m_Server->GetConnectedClientsCount() == 0)
			return;

		// Get all entities with NetowrkComponent
		auto view = scene->GetAllEntitiesWith<NetworkComponent>();
		if (view.empty())
			return;

		// Message header
		MessageEntityReplicate msgHeader;
		NetworkStreamWriter stream(m_Server->m_ScratchBuffer);
		stream.SkipBytes(sizeof(msgHeader));

		using ReplicateComponents = NetTransform::ReplicateComponents;

		// Iterate over all entities with NetworkComponent
		for (entt::entity _entity : view)
		{
			Entity entity(_entity, scene);
			auto& net = view.get<NetworkComponent>(_entity);
			auto& transform = entity.GetComponent<TransformComponent>();
			auto& netTransform = net.NetTransform;
			auto& clientTransformData = netTransform.ServerDataMap[clientID];

			// Network cull distance
			if (clientEntity)
			{
				const auto& clientTransform = clientEntity.GetComponent<TransformComponent>();
				
				float distanceToCurrent = glm::distance(
					glm::vec2{ transform.WorldPosition.x, transform.WorldPosition.y },
					glm::vec2{ clientTransform.WorldPosition.x, clientTransform.WorldPosition.y}
				);

				float distanceToLastAuthoritative = glm::distance(
					glm::vec2{ clientTransformData.Value.Position.x, clientTransformData.Value.Position.y },
					glm::vec2{ clientTransform.WorldPosition.x, clientTransform.WorldPosition.y }
				);

				if (distanceToCurrent > netTransform.CullDistance && distanceToLastAuthoritative > netTransform.CullDistance)
					continue;
			}

			// Entity payload header
			MessageEntityReplicate::PayloadItem itemHeader;
			itemHeader.EntityUUID = entity.GetUUID();
			itemHeader.ScriptCount = 0;
			auto& flags = itemHeader.TransformFlags;

			const uint64_t entityPayloadStart = stream.GetStreamPosition();
			stream.SkipBytes(sizeof(itemHeader));

			////////////////////////////// NetTransform Serialization //////////////////////////////

			bool hasRigidbodySimulated = net.SimulateOnClient && entity.HasComponent<RigidbodyComponent>();

			// Current values are stored in TransformComponent and VelocityComponent
			const auto& position = hasRigidbodySimulated ? transform.WorldPosition : transform.LocalPosition;
			const auto& scale = transform.Scale;
			const auto& rotation = transform.Rotation;

			// Previous server tick NetTransform values 
			auto& prevTransform = clientTransformData.Value;
			auto& prevPosition = prevTransform.Position;
			auto& prevScale = prevTransform.Scale;
			auto& prevRotation = prevTransform.Rotation;

			itemHeader.TransformSequenceNumber = clientTransformData.SequenceNumber;

			if (!forceReplication)
			{
				constexpr float epsilon = 1e-5f;
				#define REPLICATE_COMPONENT(current, previous, flag) \
				if (glm::epsilonNotEqual(current, previous, epsilon)) { \
					stream.WriteRaw(current); \
					previous = current; \
					flags = EnumAddFlags(flags, ReplicateComponents::flag); \
				}
				REPLICATE_COMPONENT(position.x, prevPosition.x, PositionX);
				REPLICATE_COMPONENT(position.y, prevPosition.y, PositionY);
				REPLICATE_COMPONENT(scale.x, prevScale.x, ScaleX);
				REPLICATE_COMPONENT(scale.y, prevScale.y, ScaleY);
				REPLICATE_COMPONENT(rotation, prevRotation, Rotation);
			}
			else 
			{
				stream.WriteRaw(position.x); prevPosition.x = position.x;
				stream.WriteRaw(position.y); prevPosition.y = position.y;
				stream.WriteRaw(scale.x); prevScale.x = scale.x;
				stream.WriteRaw(scale.y); prevScale.y = scale.y;
				stream.WriteRaw(rotation); prevRotation = rotation;
				flags = EnumAddFlags(flags, ReplicateComponents::All);
			}
			net.NetTransform.ReplicationFlags = flags;

			////////////////////////////// ScriptComponent Serialization //////////////////////////////
			uint64_t scriptsPayloadStart = stream.GetStreamPosition();

			for (size_t scriptIndex = 0; scriptIndex < net.ReplicatedScripts.size(); scriptIndex++)
			{
				auto& script = net.ReplicatedScripts[scriptIndex];

				if (script.ReplicatedFields.size() == 0)
					continue; // invalid entry, no fields to replicate

				uint64_t scriptPayloadItemStart = stream.GetStreamPosition();
				uint32_t fieldCount = 0;

				// Write script index (from net.ReplicatedScripts)
				stream.WriteRaw((uint32_t)scriptIndex);
				// Script replicated field count (write later)
				stream.SkipBytes(sizeof(fieldCount));

				// Iterate over each script field and write field index and data
				for (size_t fieldIndex = 0; fieldIndex < script.ReplicatedFields.size(); fieldIndex++)
				{
					auto& field = script.ReplicatedFields[fieldIndex];

					if (!forceReplication)
					{
						uint32_t checksum = crc32_bitwise(field.Data, field.Size);
						auto& lastChecksum = field.ClientToChecksumMap[clientID];
						if (checksum == lastChecksum)
							continue; // Field value not changed, skip replication
						lastChecksum = checksum;
					}

					// Write field index
					stream.WriteRaw((uint32_t)fieldIndex);
					// Write field data 
					stream.WriteData((char*)field.Data, field.Size);
					fieldCount++;
				}

				if (fieldCount > 0)
				{
					// Write replicated field count to script header
					stream.WriteRawAt(scriptPayloadItemStart + sizeof(uint32_t), fieldCount);
					itemHeader.ScriptCount++;
				}
				else
				{
					// No script replication
					stream.SetStreamPosition(scriptPayloadItemStart); 
				}
			}

			if (itemHeader.ScriptCount == 0)
			{
				// No scripts were replicated, go back to the scripts payload start position
				stream.SetStreamPosition(scriptsPayloadStart);
			}
			///////////////////////////////////////////////////////////////////////////////////////////

			if (itemHeader.TransformFlags == ReplicateComponents::None && itemHeader.ScriptCount == 0)
			{
				// Nothing was replicated, skip entity replication
				stream.SetStreamPosition(entityPayloadStart);
				continue;
			}

			// Calculate entity buffer size
			itemHeader.PayloadSize = stream.GetStreamPosition() - entityPayloadStart - sizeof(itemHeader);

			// Write entity payload header
			stream.WriteRawAt(entityPayloadStart, itemHeader);

			m_Server->m_NetStatistics->m_ReplicationStats.RepEntitiesCount++;
			msgHeader.EntityCount++;
		}

		// Write replication message header
		stream.WriteRawAt(0, msgHeader);

		if (clientID == 0)
			m_Server->SendBufferToAllClients(stream.GetBuffer());
		else
			m_Server->SendBufferToClient(clientID, stream.GetBuffer());

		m_Server->m_NetStatistics->m_ReplicationStats.RepPacketCount++;
	}
}
