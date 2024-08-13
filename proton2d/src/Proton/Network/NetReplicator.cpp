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

#include <Crc32.h>

namespace proton {

	NetReplicator::NetReplicator(Client* client)
		: m_Client(client)
	{
	}
	NetReplicator::NetReplicator(Server* server)
		: m_Server(server)
	{
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////
	////                                       Client Replicator                                       ////
	///////////////////////////////////////////////////////////////////////////////////////////////////////

	void NetReplicator::Client_ProcessReplicationMessage(BufferStreamReader& stream)
	{
		PROFILE_FUNCTION();
		Scene* scene = m_Client->m_GameInstance->GetActiveScene();

		NetMessageReplicate header;
		stream.ReadRaw(header);

		for (uint32_t i = 0; i < header.EntityCount; i++)
		{
			uint64_t entityPayloadStart = stream.GetStreamPosition();

			NetMessageReplicate::PayloadItem item;
			stream.ReadRaw(item);

			// Find entity
			Entity entity = scene->FindByID(item.EntityUUID);

			if (!entity.IsValid())
			{
				stream.SetStreamPosition(entityPayloadStart + item.PayloadSize);
				continue;
			}

			if (!entity.HasComponent<NetworkComponent>())
			{
				stream.SetStreamPosition(entityPayloadStart + item.PayloadSize);
				continue;
			}

			auto& net = entity.GetComponent<NetworkComponent>();
			auto& transform = entity.GetComponent<TransformComponent>();
			auto& velocity = entity.GetComponent<VelocityComponent>();

			auto& netTransform = net.NetTransform;
			auto& flags = item.TransformReplicationFlags;
			net.NetTransform.RepFlags = flags;
			using ReplicationFlags = NetTransform::ReplicationFlags;

			if (flags != ReplicationFlags::None)
			{
				netTransform.PreviousTransform = netTransform.CurrentTransform;
				netTransform.SyncState.PacketDelay = netTransform.SyncState.PacketTimer.Elapsed();
				netTransform.SyncState.PacketTimer.Reset();
				netTransform.SyncState.NewUpdate = true;
			}

			if (EnumHasAllFlags(flags, ReplicationFlags::All))
			{
				stream.ReadRaw(netTransform.CurrentTransform);
			}
			else
			{
				auto& position = netTransform.CurrentTransform.Position;
				if (EnumHasAllFlags(flags, ReplicationFlags::Position))
				{
					stream.ReadRaw(position);
				}
				else if (EnumHasAnyFlags(flags, ReplicationFlags::Position))
				{
					if (EnumHasAnyFlags(flags, ReplicationFlags::PositionX))
						stream.ReadRaw(position.x);
					if (EnumHasAnyFlags(flags, ReplicationFlags::PositionY))
						stream.ReadRaw(position.y);
				}

				auto& scale = netTransform.CurrentTransform.Scale;
				if (EnumHasAllFlags(flags, ReplicationFlags::Scale))
				{
					stream.ReadRaw(scale);
				}
				else if (EnumHasAnyFlags(flags, ReplicationFlags::Scale))
				{
					if (EnumHasAnyFlags(flags, ReplicationFlags::ScaleX))
						stream.ReadRaw(scale.x);
					if (EnumHasAnyFlags(flags, ReplicationFlags::ScaleY))
						stream.ReadRaw(scale.y);
				}

				auto& rotation = netTransform.CurrentTransform.Rotation;
				if (EnumHasAnyFlags(flags, ReplicationFlags::Rotation))
					stream.ReadRaw(rotation);

				auto& linearVelocity = netTransform.CurrentTransform.LinearVelocity;
				if (EnumHasAllFlags(flags, ReplicationFlags::LinearVelocity))
				{
					stream.ReadRaw(linearVelocity);
				}
				else if (EnumHasAnyFlags(flags, ReplicationFlags::LinearVelocity))
				{
					if (EnumHasAnyFlags(flags, ReplicationFlags::LinearVelocityX))
						stream.ReadRaw(linearVelocity.x);
					if (EnumHasAnyFlags(flags, ReplicationFlags::LinearVelocityY))
						stream.ReadRaw(linearVelocity.y);
				}

				auto& angularVelocity = netTransform.CurrentTransform.AngularVelocity;
				if (EnumHasAnyFlags(flags, ReplicationFlags::AngularVelocity))
					stream.ReadRaw(angularVelocity);
			}

			// For each script
			for (uint32_t i = 0; i < item.ScriptCount; i++)
			{
				uint32_t scriptIndex;
				uint32_t fieldCount;

				stream.ReadRaw(scriptIndex);
				stream.ReadRaw(fieldCount);

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

	void NetReplicator::Client_OnEntitySpawnMessage(BufferStreamReader& stream)
	{
		PROFILE_FUNCTION();
		Scene* scene = m_Client->m_GameInstance->GetActiveScene();

		NetMessageSpawn header;
		stream.ReadRaw(header);

		for (uint32_t i = 0; i < header.EntityCount; i++)
		{
			NetMessageSpawn::PayloadItem item;
			stream.ReadString(item.EntityJsonData);

			json jsonParsed = json::parse(item.EntityJsonData);

			if (scene->FindByID((UUID)jsonParsed.at("UUID")))
				break;

			SceneSerializer serializer(scene);
			Entity entity = serializer.DeserializeEntity(jsonParsed);
		}
	}

	void NetReplicator::Client_OnEntityDespawnMessage(BufferStreamReader& stream)
	{
		PROFILE_FUNCTION();
		Scene* scene = m_Client->m_GameInstance->GetActiveScene();

		NetMessageDespawn header;
		stream.ReadRaw(header);

		for (uint32_t i = 0; i < header.EntityCount; i++)
		{
			NetMessageDespawn::PayloadItem item;
			stream.ReadRaw(item);

			Entity entity = scene->FindByID(item.EntityUUID);
			if (!entity)
				continue;

			entity.Destroy();
		}
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////
	////                                       Server Replicator                                       ////
	///////////////////////////////////////////////////////////////////////////////////////////////////////

	void NetReplicator::Server_OnUpdate()
	{
		PROFILE_FUNCTION();
		Scene* scene = m_Server->m_GameInstance->GetActiveScene();
		Server_ProcessSpawnedEntityQueue(scene);
		Server_ProcessDespawnedEntityQueue(scene);
		//Server_SendReplicationMessage();

		for (auto [clientID, clientInfo] : m_Server->m_ConnectedClients)
		{
			if (clientInfo.Status == ConnectionStatus::Connected)
			{
				Server_SendReplicationMessage(clientID);
			}
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
		SceneData& sceneData = m_SceneData[scene];
		sceneData.SpawnedEntityQueue.push(entityUUID);
	}

	void NetReplicator::Server_AddDespawnedEntity(Scene* scene, UUID entityUUID)
	{
		SceneData& sceneData = m_SceneData[scene];
		sceneData.DespawnedEntityQueue.push(entityUUID);
	}

	void NetReplicator::Server_ProcessSpawnedEntityQueue(Scene* scene)
	{
		PROFILE_FUNCTION();
		SceneData& sceneData = m_SceneData[scene];
		if (sceneData.SpawnedEntityQueue.empty())
			return;

		NetMessageSpawn header;
		BufferStreamWriter stream(m_Server->m_ScratchBuffer);
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

			// TODO: Change to PrefabUUID and EntityUUID
			SceneSerializer serializer(entity.m_Scene, true);
			stream.WriteString(serializer.SerializeEntityToString(entity));
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
		SceneData& sceneData = m_SceneData[scene];
		if (sceneData.DespawnedEntityQueue.empty())
			return;

		NetMessageDespawn header;
		BufferStreamWriter stream(m_Server->m_ScratchBuffer);
		stream.SkipBytes(sizeof(header));

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
			header.EntityCount = despawned;
			stream.WriteRawAt(0, header);
			m_Server->SendBufferToAllClients(stream.GetBuffer());
		}
	}

	void NetReplicator::Server_SendAllSpawnedEntities(Scene* scene, ClientID clientID)
	{
		PROFILE_FUNCTION();
		SceneData& sceneData = m_SceneData[scene];
		auto& spawnedAll = sceneData.SpawnedAll;
		if (spawnedAll.size() > 0)
		{
			BufferStreamWriter stream(m_Server->m_ScratchBuffer);

			NetMessageSpawn msg;
			msg.EntityCount = (uint32_t)spawnedAll.size();
			stream.WriteRaw(msg);

			for (UUID uuid : spawnedAll)
			{
				NetMessageSpawn::PayloadItem item;
				Entity entity = scene->FindByID(uuid);
				SceneSerializer serializer(scene, true);
				item.EntityJsonData = serializer.SerializeEntityToString(entity);
				stream.WriteString(item.EntityJsonData);
			}

			m_Server->SendBufferToClient(clientID, stream.GetBuffer());
		}
	}

	void NetReplicator::Server_SendAllDespawnedEntities(Scene* scene, ClientID clientID)
	{
		PROFILE_FUNCTION();
		SceneData& sceneData = m_SceneData[scene];
		auto& despawnedAll = sceneData.DespawnedAll;
		if (despawnedAll.size() > 0)
		{
			BufferStreamWriter stream(m_Server->m_ScratchBuffer);

			NetMessageDespawn msg;
			msg.EntityCount = (uint32_t)despawnedAll.size();
			stream.WriteRaw(msg);

			for (UUID uuid : despawnedAll)
			{
				NetMessageDespawn::PayloadItem item;
				item.EntityUUID = uuid;
				stream.WriteRaw(item);
			}

			m_Server->SendBufferToClient(clientID, stream.GetBuffer());
		}
	}

	void NetReplicator::Server_SendReplicationMessage(ClientID clientID, bool forceReplication)
	{
		PROFILE_FUNCTION();

		// Replicate entitites with NetworkComponent
		Scene* scene = m_Server->m_GameInstance->GetActiveScene();
		auto view = scene->GetAllEntitiesWith<NetworkComponent>();

		if (view.empty() || m_Server->GetConnectedClientsCount() == 0)
			return;

		NetMessageReplicate msgHeader;
		msgHeader.EntityCount = 0;

		BufferStreamWriter stream(m_Server->m_ScratchBuffer);
		stream.SkipBytes(sizeof(msgHeader));

		using ReplicationFlags = NetTransform::ReplicationFlags;

		// Iterate over entities with NetworkComponent
		for (entt::entity _entity : view)
		{
			Entity entity(_entity, scene);
			auto& net = view.get<NetworkComponent>(_entity);
			auto& transform = entity.GetComponent<TransformComponent>();
			auto& velocity = entity.GetComponent<VelocityComponent>();

			if (Entity clientEntity = m_Server->GetClientEntity(clientID))
			{
				auto& clientTransform = clientEntity.GetComponent<TransformComponent>();
				float distance = glm::distance(
					glm::vec2{ transform.WorldPosition.x, transform.WorldPosition.y },
					glm::vec2{ clientTransform.WorldPosition.x, clientTransform.WorldPosition.y}
				);

				if (distance > net.CullDistance)
					continue;
			}

			// Entity payload header
			NetMessageReplicate::PayloadItem itemHeader;
			itemHeader.EntityUUID = entity.GetUUID();
			itemHeader.ScriptCount = 0;
			auto& flags = itemHeader.TransformReplicationFlags;
			flags = ReplicationFlags::None;

			uint64_t entityPayloadStart = stream.GetStreamPosition();
			stream.SkipBytes(sizeof(itemHeader));

			////////////////////////////// NetTransform Serialization //////////////////////////////

			auto& netTransform = net.NetTransform;
			bool rbSync = netTransform.SyncParams.SyncMethod == NetSyncMethod::NetworkRigidbody;

			// Current values are stored in TransformComponent and VelocityComponent
			const auto& position = rbSync ? transform.WorldPosition : transform.LocalPosition;
			const auto& scale = transform.Scale;
			const auto& rotation = transform.Rotation;
			const auto& linearVelocity = velocity.LinearVelocity;
			const auto& angularVelocity = velocity.AngularVelocity;

			// Previous server tick NetTransform values 
			auto& prevTransform = netTransform.ClientToTransformMap[clientID];
			auto& prevPosition = prevTransform.Position;
			auto& prevScale = prevTransform.Scale;
			auto& prevRotation = prevTransform.Rotation;
			auto& prevLinearVelocity = prevTransform.LinearVelocity;
			auto& prevAngularVelocity = prevTransform.AngularVelocity;

			if (!forceReplication)
			{
				constexpr float epsilon = 0.00001f;
				#define REPLICATE_COMPONENT(current, previous, flag) \
				if (glm::epsilonNotEqual(current, previous, epsilon)) { \
					stream.WriteRaw(current); \
					previous = current; \
					flags = EnumAddFlags(flags, ReplicationFlags::flag); \
				}
				REPLICATE_COMPONENT(position.x, prevPosition.x, PositionX);
				REPLICATE_COMPONENT(position.y, prevPosition.y, PositionY);
				REPLICATE_COMPONENT(scale.x, prevScale.x, ScaleX);
				REPLICATE_COMPONENT(scale.y, prevScale.y, ScaleY);
				REPLICATE_COMPONENT(rotation, prevRotation, Rotation);
				REPLICATE_COMPONENT(linearVelocity.x, prevLinearVelocity.x, LinearVelocityX);
				REPLICATE_COMPONENT(linearVelocity.y, prevLinearVelocity.y, LinearVelocityY);
				REPLICATE_COMPONENT(angularVelocity, prevAngularVelocity, AngularVelocity);
			}
			else 
			{
				stream.WriteRaw(position.x); prevPosition.x = position.x;
				stream.WriteRaw(position.y); prevPosition.y = position.y;
				stream.WriteRaw(scale.x); prevScale.x = scale.x;
				stream.WriteRaw(scale.y); prevScale.y = scale.y;
				stream.WriteRaw(rotation); prevRotation = rotation;
				stream.WriteRaw(linearVelocity); prevLinearVelocity = linearVelocity;
				stream.WriteRaw(angularVelocity); prevAngularVelocity = angularVelocity;
				flags = EnumAddFlags(flags, ReplicationFlags::All);
			}
			net.NetTransform.RepFlags = flags;

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
				// No scripts were replicated, go back to payload header position
				stream.SetStreamPosition(scriptsPayloadStart);
			}
			///////////////////////////////////////////////////////////////////////////////////////////

			if (itemHeader.TransformReplicationFlags == ReplicationFlags::None && itemHeader.ScriptCount == 0)
			{
				// Nothing was replicated, skip entity replication
				stream.SetStreamPosition(entityPayloadStart);
				continue;
			}

			// Calculate entity buffer size
			itemHeader.PayloadSize = stream.GetStreamPosition() - entityPayloadStart;

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
