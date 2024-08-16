#include "ptpch.h"
#include "Proton/Network/NetTransformSystem.h"
#include "Proton/Network/Client.h"
#include "Proton/Network/Server.h"
#include "Proton/Network/Messages.h"
#include "Proton/Network/NetworkManager.h"

#include "Proton/Core/Application.h"
#include "Proton/Core/GameInstance.h"
#include "Proton/Physics/PhysicsWorld.h"

#include <box2d/b2_world.h>
#include <glm/gtx/norm.hpp>

namespace proton {

	NetTransformSystem::NetTransformSystem(Client* client)
		: m_Client(client), m_NetworkManager(client->m_NetworkManager)
	{
	}

	NetTransformSystem::NetTransformSystem(Server* server)
		: m_Server(server), m_NetworkManager(server->m_NetworkManager)
	{
	}

	void NetTransformSystem::Client_SendSequenceNumberMessage()
	{
		PROFILE_FUNCTION();
		
		if (m_SequenceNumbersToSend.empty())
			return;

		NetworkStreamWriter stream(m_Client->m_ScratchBuffer);
		NetMessageTransformSequenceNumber header;
		stream.SkipBytes(sizeof(header));

		for (const auto& item : m_SequenceNumbersToSend)
		{
			stream.WriteRaw(item);
			header.EntityCount++;
		}
		
		stream.WriteRawAt(0, header);
		m_Client->SendBuffer(stream.GetBuffer());
		m_SequenceNumbersToSend.clear();
	}

	void NetTransformSystem::Server_OnSequenceNumberMessage(ClientID clientID, NetworkStreamReader& stream)
	{
		PROFILE_FUNCTION();
		
		NetMessageTransformSequenceNumber header;
		stream.ReadRaw(header);

		// Get active scene based on client entity
		auto clientEntityIt = m_Server->m_ClientToEntityMap.find(clientID);
		Scene* scene = clientEntityIt != m_Server->m_ClientToEntityMap.end() 
			? clientEntityIt->second.GetScene() : m_Server->m_GameInstance->GetActiveScene();
		
		for (uint32_t i = 0; i < header.EntityCount; i++)
		{
			NetMessageTransformSequenceNumber::PayloadItem item;
			if (!stream.ReadRaw(item))
			{
				PT_THROW_ERROR("Failed to read sequence number: Stream out of memory (client_id={}, entity_count={}, buffer_size={})",
					clientID, header.EntityCount, stream.GetTargetBuffer().Size);

				m_Server->KickClient(clientID);
				return;
			}

			if (Entity entity = scene->FindByID(item.EnittyUUID))
			{
				if (!entity.HasComponent<NetworkComponent>())
				{
					PT_THROW_ERROR("Entity {} does not have NetworkComponent (client_id={})", item.EnittyUUID, clientID);
					continue;
				}

				auto& net = entity.GetComponent<NetworkComponent>();
				auto& data = net.NetTransform.ClientDataMap[clientID];
				data.SequenceNumber = item.SequenceNumber;
			}
		}
	}

	static inline bool IsWithinPrecision(const glm::vec2& currentDelta, const glm::vec2& predictedDelta)
	{
		constexpr float precision = 8.0f;
		const float predictedDeltaMax = glm::compMax(glm::abs(predictedDelta));
		const float currentDeltaMax = glm::compMax(glm::abs(currentDelta));
		return predictedDeltaMax > 1e-6f && currentDeltaMax < predictedDeltaMax * precision;
	}

	void NetTransformSystem::OnUpdate(Scene* scene, float ts)
	{
		PROFILE_FUNCTION();
		
		using Transform = NetTransform::Transform;
		using ReconcileState = NetTransform::ReconcileState;

		Timer testTimer;
		
		auto view = scene->GetAllEntitiesWith<NetworkComponent, TransformComponent, VelocityComponent>();
		for (auto _entity : view)
		{
			Entity entity(_entity, scene);
			auto [net, transform, velocity] = view.get<NetworkComponent, TransformComponent, VelocityComponent>(_entity);
			
			auto& netTransform = net.NetTransform;
			auto& lastAuth = netTransform.LastAuthoritativeTransform;
			auto& prevAuth = netTransform.PrevAuthoritativeTransform;
			auto& predictedTransform = netTransform.PredictedTransform;

			bool isReconciling = EnumHasAnyFlags(netTransform.State, ReconcileState::All);

			switch (netTransform.Method)
			{
			case NetTransform::SyncMethod::None:
			{
				if (!netTransform.ReplicatedThisFrame)
					break;

				// Immediately set authoritative transform values
				if (EnumHasAnyFlags(net.NetTransform.Flags, NetTransform::ReplicationFlags::Position))
				{
					entity.SetLocalPosition({ lastAuth.Position.x, lastAuth.Position.y, transform.LocalPosition.z });
				}
				if (EnumHasAnyFlags(net.NetTransform.Flags, NetTransform::ReplicationFlags::Scale))
				{
					transform.Scale = { lastAuth.Scale.x, lastAuth.Scale.y };
				}
				if (EnumHasAnyFlags(net.NetTransform.Flags, NetTransform::ReplicationFlags::Rotation))
				{
					transform.Rotation = lastAuth.Rotation;
				}
				break;
			}
			case NetTransform::SyncMethod::Interpolation:
			{
				// Elapsed time from last authorative state update
				float elapsed = netTransform.ReplicationTimer.Elapsed();

				if (elapsed < netTransform.LastReplicationInterval)
				{
					// Interpolate between last two authoritative transforms
					float alpha = glm::clamp(elapsed / netTransform.LastReplicationInterval, 0.0f, 1.0f);

					glm::vec2 interpolated = glm::mix(prevAuth.Position, lastAuth.Position, alpha);
					entity.SetLocalPosition({interpolated.x, interpolated.y, transform.LocalPosition.z});
					transform.Rotation = glm::mix(prevAuth.Rotation, lastAuth.Rotation, alpha);
				}
				// If next replication message did not arrive at time, extrapolate for a maximum time of 0.5s. 
				else if (elapsed < 0.5f)
				{
					transform.LocalPosition.x += velocity.LinearVelocity.x * ts;
					transform.LocalPosition.y += velocity.LinearVelocity.y * ts;
					transform.Rotation += velocity.AngularVelocity * ts;
				}
				break;
			}
			case NetTransform::SyncMethod::Extrapolation:
			{
				break;
			}
			case NetTransform::SyncMethod::Prediction:
			{
				if (!entity.HasComponent<RigidbodyComponent>())
					break;

				b2Body* body = entity.GetRuntimeBody();
				if (!body)
					break;

				// Get current transform values and calculate last tick delta
				const Transform currentTransform = Transform::Get(&transform);
				const Transform tickDelta = currentTransform - netTransform.LastTickTransform;
				auto& deltaBuffer = netTransform.DeltaBuffer;

				if (m_NetworkManager->IsNetworkTick())
				{
					netTransform.LastTickTransform = currentTransform;

					// Push current delta and sequence number to the buffer
					if (!tickDelta.IsZero())
					{
						// Increment sequence number
						netTransform.CurrentSequenceNumber++;
						deltaBuffer.push_back({ netTransform.CurrentSequenceNumber, tickDelta });

						// Send current sequence number to server (subtract 1 to lead before server data)
						m_SequenceNumbersToSend.push_back({ entity.GetUUID(), (uint16_t)(netTransform.CurrentSequenceNumber - 1) });
					}
				}

				// When authoritative transform is received from server
				if (netTransform.ReplicatedThisFrame)
				{
					// Remove deltas that happened before last authoritative transform 
					while (!deltaBuffer.empty() && deltaBuffer.front().SequenceNumber < netTransform.ServerSequenceNumber)
						deltaBuffer.erase(deltaBuffer.begin());
						//deltaBuffer.pop_front();

					// Apply deltas not processed by server yet
					predictedTransform = lastAuth;
					for (const auto& delta : deltaBuffer)
					{
						predictedTransform.Position += delta.Value.Position;
						predictedTransform.Scale += delta.Value.Scale;
						predictedTransform.Rotation += delta.Value.Rotation;
					}
					
					// Calculate deltas to the last authoritative transform
					// If current delta is small enough, do not reconcile.
					Transform deltaCurrent = lastAuth - currentTransform;
					Transform deltaPredicted = lastAuth - predictedTransform;

					if (!IsWithinPrecision(deltaCurrent.Position, deltaPredicted.Position))
					{
						//netTransform.LastTickTransform = predictedTransform;
						netTransform.State = EnumAddFlags(netTransform.State, ReconcileState::Position);
					}
				}

				// Perform reconcilation to the predicted transform
				if (EnumHasAnyFlags(netTransform.State, ReconcileState::Position))
				{
					body->SetTransform({ predictedTransform.Position.x, predictedTransform.Position.y }, 0);

					float distanceError = glm::distance(
						glm::vec2{ currentTransform.Position.x, currentTransform.Position.y },
						glm::vec2{ predictedTransform.Position.x, predictedTransform.Position.y }
					);

					if (distanceError < 0.005f)
					{
						netTransform.State = EnumRemoveFlags(netTransform.State, ReconcileState::Position);
					}
				}

				break;
#if 0
				if (!scene->m_PhysicsTick)
					break;

				if (!entity.HasComponent<RigidbodyComponent>())
					break;

				auto& rb = entity.GetComponent<RigidbodyComponent>();
				if (rb.Type == b2_staticBody)
					break;

				b2Body* body = rb.RuntimeBody;
				if (!body || syncParams.SyncMethod != NetSyncMethod::NetworkRigidbody)
					break;

				float lastPacketElapsed = glm::min(syncState.PacketDelay, 1.0f / 16.0f);
				float multiplier = lastPacketElapsed + Server::s_FakeServerLag / 1000.0f;

				syncState.ExtrapolatedPoint = {
					current.Position.x + velocity.LinearVelocity.x * multiplier,
					current.Position.y + velocity.LinearVelocity.y * multiplier
				};

				if (!syncState.ReconcileStarted && syncState.ReconcileCooldownTimer.Elapsed() > syncParams.ReconcileCooldownTime)
				{
					float distance = glm::distance(
						glm::vec2{ transform.WorldPosition.x, transform.WorldPosition.y },
						glm::vec2{ syncState.ExtrapolatedPoint.x, syncState.ExtrapolatedPoint.y }
					);
					syncState.Error = distance;

					if (syncState.Error >= syncParams.ReconcileThreshold)
					{
						if (entity.GetTag() == "Player")
							_PT_CORE_TRACE("Reconcile: len2={:.3f}, dist={:.3}, mul={:.3f}", syncState.Error, distance, multiplier);

						syncState.ReconcileStarted = true;
						syncState.ReconcileCooldownTimer.Reset();

						//body->SetGravityScale(0.0f);
					}
				}

				if (syncState.ReconcileStarted)
				{
					body->SetTransform({ syncState.ExtrapolatedPoint.x, syncState.ExtrapolatedPoint.y }, 0);

					float distanceError = glm::distance(
						glm::vec2{ transform.WorldPosition.x, transform.WorldPosition.y },
						glm::vec2{ syncState.ExtrapolatedPoint.x, syncState.ExtrapolatedPoint.y }
					);
					//float distanceError = glm::length2(syncState.ExtrapolatedPoint - transform.WorldPosition);

					if (distanceError < 0.005f)
					{
						syncState.ReconcileStarted = false;
						//body->SetGravityScale(1.0f);
					}	
				}
				break;
#endif
			}
			}

			// Reset replication state
			netTransform.ReplicatedThisFrame = false;
		}

		//_PT_CORE_TRACE("{:.9f}", testTimer.ElapsedMillis());

		if (m_Client && m_NetworkManager->IsNetworkTick())
		{
			Client_SendSequenceNumberMessage();
		}
	}
}
