#include "ptpch.h"
#include "Proton/Network/ReplicationManager.h"
#include "Proton/Network/Server.h"
#include "Proton/Network/NetStatistics.h"
#include "Proton/Network/Packets.h"
#include "Proton/Scene/Scene.h"
#include "Proton/Scene/Entity.h"

#include <Crc32.h>

namespace proton {

	ReplicationManager::ReplicationManager(Server* server)
		: m_Server(server)
	{
	}

#define BEGIN_COMPONENT_BUFFER_WRITE(__component_type)                             \
	if (net.ComponentsToReplicate.test(__component_type)) {                        \
		EComponentType _component_type = __component_type;                         \
		uint64_t _streamStart = stream.GetStreamPosition();						
																				
#define END_COMPONENT_BUFFER_WRITE()                                               \
		if (verifyComponentChecksum) {											   \
			uint64_t _streamEnd = stream.GetStreamPosition();                      \
			uint64_t _streamSize = _streamEnd - _streamStart;                      \
			void* _dataBuffer = (char*)stream.GetBuffer().Data + _streamStart;     \
			uint32_t _checksum = crc32_bitwise(_dataBuffer, _streamSize);          \
			uint32_t& _previousChecksum = net.ComponentChecksum[_component_type];  \
			if (_checksum == _previousChecksum) {                                  \
				stream.SetStreamPosition(_streamStart);                            \
				replicatedComponentsBitset.set(_component_type, false);            \
			}                                                                      \
			else _previousChecksum = _checksum;                                    \
		}                                                                          \
	}

	void ReplicationManager::StreamWriteReplicationData(BufferStreamWriter& stream, Scene* scene, bool verifyComponentChecksum)
	{
		PROFILE_FUNCTION();
		// Replicate entitites with NetworkComponent
		auto view = scene->GetAllEntitiesWith<NetworkComponent>();
		if (view.empty() || m_Server->m_ConnectedClients.size() == 0)
			return;

		NetMessageReplicate msg;
		msg.EntityCount = 0;

		uint64_t streamStart = stream.GetStreamPosition();
		stream.SkipBytes(sizeof(NetMessageReplicate));

		// Iterate over entities with NetworkComponent
		for (entt::entity _entity : view)
		{
			PROFILE_SCOPE("replicate_entity");
			Entity entity(_entity, scene);
			auto& net = view.get<NetworkComponent>(_entity);

			// Entity entry header
			NetMessageReplicate_Entry msgEntry;
			msgEntry.EntityUUID = entity.GetUUID();
			auto& replicatedComponentsBitset = msgEntry.ComponentBitset;
			replicatedComponentsBitset = net.ComponentsToReplicate;

			uint64_t entityHeaderPos = stream.GetStreamPosition();
			stream.SkipBytes(sizeof(NetMessageReplicate_Entry));

			// ------------ Component binary serialization ------------
			uint64_t componentsBegin = stream.GetStreamPosition();

			// TransformComponent
			BEGIN_COMPONENT_BUFFER_WRITE(ComponentType_Transform);
			{
				auto& transform = entity.GetComponent<TransformComponent>();
				bool rbSync = net.SyncParams.SyncMethod == NetSyncMethod::NetworkRigidbody;
				stream.WriteRaw(rbSync ? transform.WorldPosition : transform.LocalPosition);
				stream.WriteRaw(transform.Rotation);
			}
			END_COMPONENT_BUFFER_WRITE();

			// VelocityComponent
			BEGIN_COMPONENT_BUFFER_WRITE(ComponentType_Velocity);
			{
				auto& velocity = entity.GetComponent<VelocityComponent>();
				stream.WriteRaw(velocity.LinearVelocity);
				stream.WriteRaw(velocity.AngularVelocity);
			}
			END_COMPONENT_BUFFER_WRITE();

			// ScriptComponent
			if (net.ComponentsToReplicate.test(ComponentType_Script))
			{
				// Component buffer header
				uint64_t componentHeaderPos = stream.GetStreamPosition();
				uint32_t scriptRepCount = 0;

				stream.WriteZero(sizeof(scriptRepCount));

				for (size_t scriptIndex = 0; scriptIndex < net.ReplicatedScripts.size(); scriptIndex++)
				{
					auto& script = net.ReplicatedScripts[scriptIndex];
					if (script.ReplicatedFields.size() == 0)
						continue; // invalid entry, no fields to replicate

					// Script buffer header
					uint64_t scriptHeaderPos = stream.GetStreamPosition();
					uint32_t fieldRepCount = 0;

					stream.WriteZero(sizeof(fieldRepCount));
					stream.WriteRaw((uint32_t)scriptIndex);

					// For each script field
					for (size_t fieldIndex = 0; fieldIndex < script.ReplicatedFields.size(); fieldIndex++)
					{
						auto& field = script.ReplicatedFields[fieldIndex];

						if (verifyComponentChecksum)
						{
							uint32_t checksum = crc32_bitwise(field.Data, field.Size);
							if (checksum == field.Checksum)
								continue; // field value not changed, skip replication
							field.Checksum = checksum;
						}

						// Write field data
						stream.WriteRaw((uint32_t)fieldIndex);
						stream.WriteData((char*)field.Data, field.Size);
						fieldRepCount++;
					}

					if (fieldRepCount)
					{
						// Write field count to script header
						uint64_t current = stream.GetStreamPosition();
						stream.SetStreamPosition(scriptHeaderPos);
						stream.WriteRaw(fieldRepCount);
						stream.SetStreamPosition(current);
						scriptRepCount++;
					}
					else
						stream.SetStreamPosition(scriptHeaderPos); // no fields to replicate
				}

				if (scriptRepCount)
				{
					// Write script count to component header
					uint64_t streamPos = stream.GetStreamPosition();
					stream.SetStreamPosition(componentHeaderPos);
					stream.WriteRaw(scriptRepCount);
					stream.SetStreamPosition(streamPos);
				}
				else
				{
					// Skip ScriptComponent replication
					stream.SetStreamPosition(componentHeaderPos);
					replicatedComponentsBitset.set(ComponentType_Script, false);
				}
			}

			// Check if any components were written to stream buffer
			if (stream.GetStreamPosition() == componentsBegin)
			{
				stream.SetStreamPosition(entityHeaderPos);
				continue;
			}

			// Calculate buffer size
			uint64_t entityStreamEnd = stream.GetStreamPosition();
			msgEntry.PayloadSize = entityStreamEnd - entityHeaderPos;

			// Write entity header
			stream.SetStreamPosition(entityHeaderPos);
			stream.WriteRaw(msgEntry);
			stream.SetStreamPosition(entityStreamEnd);

			m_Server->m_NetStatistics->m_ReplicationStats.RepEntitiesCount++;
			msg.EntityCount++;
		}

		uint64_t bufferPos = stream.GetStreamPosition();
		stream.SetStreamPosition(streamStart);
		stream.WriteRaw(msg); 
		stream.SetStreamPosition(bufferPos);
	}
}
