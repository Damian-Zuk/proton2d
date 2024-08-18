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

	using Transform = NetTransform::Transform;
	using ReconcileState = NetTransform::ReconcileState;

	NetTransformSystem::NetTransformSystem(Client* client)
		: m_Client(client), m_NetworkManager(client->m_NetworkManager)
	{
	}

	NetTransformSystem::NetTransformSystem(Server* server)
		: m_Server(server), m_NetworkManager(server->m_NetworkManager)
	{ 
	}

	void NetTransformSystem::OnUpdate(Scene* scene, float ts)
	{
		PROFILE_FUNCTION();

		// Timer testTimer;
		auto view = scene->GetAllEntitiesWith<NetworkComponent, TransformComponent>();
		for (auto _entity : view)
		{
			Entity entity(_entity, scene);
			auto [net, transform] = view.get<NetworkComponent, TransformComponent>(_entity);

			// Try to retrieve Box2D rigidbody `b2Body*`
			bool isRigidbodySimulated = net.SimulateOnClient && entity.HasComponent<RigidbodyComponent>();
			b2Body* rigidbody = entity.GetRuntimeBody();
			if (isRigidbodySimulated && !rigidbody)
				continue; // Runtime body not created yet, skip replication tick
			
			// `NetTransform` member references
			auto& netTransform = net.NetTransform;
			auto& lastAuthorative = netTransform.LastAuthoritativeTransform;
			auto& prevAuthorative = netTransform.PrevAuthoritativeTransform;
			auto& predictedTransform = netTransform.PredictedTransform;
			auto& replicationTimer = netTransform.ReplicationTimer;
			auto& replicationInterval = netTransform.ReplicationInterval;

			// `replicationTimer` is being reset by `NetReplicator`
			bool replicatedThisFrame = replicationTimer == 0.0f;

			// Handle transform sync for each method
			switch (netTransform.Method)
			{
			////////////////////////////////////////////////////////////////////////////////////////////////////
			case NetTransform::SyncMethod::None:
			{
				if (replicatedThisFrame)
					break;

				// Immediately set authoritative transform values
				if (EnumHasAnyFlags(net.NetTransform.Flags, NetTransform::ReplicationFlags::Position))
				{
					entity.SetLocalPosition({ lastAuthorative.Position.x, lastAuthorative.Position.y, transform.LocalPosition.z });
				}
				if (EnumHasAnyFlags(net.NetTransform.Flags, NetTransform::ReplicationFlags::Scale))
				{
					transform.Scale = { lastAuthorative.Scale.x, lastAuthorative.Scale.y };
				}
				if (EnumHasAnyFlags(net.NetTransform.Flags, NetTransform::ReplicationFlags::Rotation))
				{
					transform.Rotation = lastAuthorative.Rotation;
				}
				break;
			}
			////////////////////////////////////////////////////////////////////////////////////////////////////
			case NetTransform::SyncMethod::Interpolation:
			{
				// Interpolate between last two authoritative transforms
				// Calculate alpha
				float alpha = glm::clamp(replicationTimer / replicationInterval, 0.0f, 1.0f);
				glm::vec2 interpolatedPosition = glm::mix(prevAuthorative.Position, lastAuthorative.Position, alpha);
				float interpolatedRotation = glm::mix(prevAuthorative.Rotation, lastAuthorative.Rotation, alpha);
				
				if (rigidbody != nullptr)
				{
					rigidbody->SetTransform({ interpolatedPosition.x, interpolatedPosition.y }, interpolatedRotation * (b2_pi / 180.0f));
				}
				else
				{
					entity.SetLocalPosition(interpolatedPosition);
					transform.Scale = glm::mix(prevAuthorative.Scale, lastAuthorative.Scale, alpha);
					transform.Rotation = interpolatedRotation;
				}
				break;
			}
			////////////////////////////////////////////////////////////////////////////////////////////////////
			case NetTransform::SyncMethod::Extrapolation:
			{
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
#endif
				break;
			}
			////////////////////////////////////////////////////////////////////////////////////////////////////
			case NetTransform::SyncMethod::Prediction:
			{
				// Get current transform values and calculate last tick delta (world space if uses physics body)
				const Transform currentTransform = Transform::Get(&transform);
				auto& deltaBuffer = netTransform.DeltaBuffer;

				// Push current delta and sequence number to the buffer
				if (m_NetworkManager->IsNetworkTick())
				{
					// Calculate this tick delta
					Transform thisTickDelta = currentTransform - netTransform.LastTickTransform;
					
					// Update last tick transform
					netTransform.LastTickTransform = currentTransform;

					// Ignore delta if reconciling
					if (netTransform.IsReconciling(ReconcileState::Position))
						thisTickDelta.Position = glm::vec2{ 0.0f, 0.0f };

					if (netTransform.IsReconciling(ReconcileState::Scale))
						thisTickDelta.Scale = glm::vec2{ 0.0f, 0.0f };

					if (netTransform.IsReconciling(ReconcileState::Rotation))
						thisTickDelta.Rotation = 0.0f;
					
					// Store delta and new sequence number if delta not zero
					if (thisTickDelta.IsNotZero())
					{
						// Increment sequence number, store in deltaBuffer and send current sequence number to server
						netTransform.CurrentSequenceNumber++;
						deltaBuffer.push_back({ netTransform.CurrentSequenceNumber, thisTickDelta });
						m_SequenceNumbersToSend.push_back({ entity.GetUUID(), netTransform.CurrentSequenceNumber });
					}
				}

				// Recalculate predicted transform and check if need to reconcile
				if (replicatedThisFrame)
				{
					// Remove deltas that happened before last authoritative transform 
					while (!deltaBuffer.empty() && deltaBuffer.front().SequenceNumber <= netTransform.ServerSequenceNumber)
					{
						deltaBuffer.erase(deltaBuffer.begin());
						//deltaBuffer.pop_front();
					}

					// Apply deltas not processed by server yet
					predictedTransform = lastAuthorative;
					for (const auto& delta : deltaBuffer)
					{
						predictedTransform.Position += delta.Value.Position;
						predictedTransform.Scale += delta.Value.Scale;
						predictedTransform.Rotation += delta.Value.Rotation;
					}

					// Check position error
					float positionError = glm::distance(currentTransform.Position, predictedTransform.Position);
					if (positionError > netTransform.ReconcileThreshold)
						netTransform.StartReconcile(ReconcileState::Position);

					// Check scale error
					float scaleError = glm::distance(currentTransform.Scale, predictedTransform.Scale);
					if (scaleError > netTransform.ReconcileThreshold)
						netTransform.StartReconcile(ReconcileState::Scale);

					// Check rotation error
					constexpr float rotationReconcileThreshold = 5.0f;
					float rotationError = glm::abs(currentTransform.Rotation - predictedTransform.Rotation);
					if (rotationError > rotationReconcileThreshold)
						netTransform.StartReconcile(ReconcileState::Rotation);
				}

				// -------------------------------------------------- Transform reconcilation ----------------------------------------------------

				// Reconcilation for physics body (scale not implemented yet)
				if (rigidbody != nullptr)
				{
					constexpr float positionErrorThreshold = 0.01f;
					constexpr float rotationErrorThreshold = 1.0f;

					// Reconcile position and rotation
					if (netTransform.IsReconciling(ReconcileState::PositionAndRotation))
					{
						float positionError = glm::distance(currentTransform.Position, predictedTransform.Position);
						if (positionError < positionErrorThreshold)
							netTransform.StopReconcile(ReconcileState::Position);

						float rotationError = glm::abs(currentTransform.Rotation - predictedTransform.Rotation);
						if (rotationError < rotationErrorThreshold)
							netTransform.StopReconcile(ReconcileState::Rotation);

						rigidbody->SetTransform({ predictedTransform.Position.x, predictedTransform.Position.y }, predictedTransform.Rotation * (b2_pi / 180.0f));
					}
					// Reconcile position only
					else if (netTransform.IsReconciling(ReconcileState::Position))
					{
						float positionError = glm::distance(currentTransform.Position, predictedTransform.Position);
						if (positionError < positionErrorThreshold)
							netTransform.StopReconcile(ReconcileState::Position);

						rigidbody->SetTransform({ predictedTransform.Position.x, predictedTransform.Position.y }, rigidbody->GetAngle());
					}
					// Reconcile rotation only
					else if (netTransform.IsReconciling(ReconcileState::Rotation))
					{
						float rotationError = glm::abs(currentTransform.Rotation - predictedTransform.Rotation);
						if (rotationError < rotationErrorThreshold)
							netTransform.StopReconcile(ReconcileState::Rotation);

						rigidbody->SetTransform({ transform.WorldPosition.x, transform.WorldPosition.y }, predictedTransform.Rotation * (b2_pi / 180.0f));
					}
				}
				else // Reconcilation for TransformComponent (with interpolation for smoothness)
				{
					if (netTransform.IsReconciling(ReconcileState::Position))
					{
						float alpha = glm::clamp(replicationTimer / netTransform.ReconcileMaxTime, 0.0f, 1.0f);
						glm::vec2 interpolated = glm::mix(currentTransform.Position, predictedTransform.Position, alpha);
						entity.SetLocalPosition(interpolated);

						if (alpha == 1.0f)
							netTransform.StopReconcile(ReconcileState::Position);
					}

					if (netTransform.IsReconciling(ReconcileState::Scale))
					{
						float alpha = glm::clamp(replicationTimer / netTransform.ReconcileMaxTime, 0.0f, 1.0f);
						glm::vec2 interpolated = glm::mix(currentTransform.Scale, predictedTransform.Scale, alpha);
						transform.Scale = interpolated;

						if (alpha == 1.0f)
							netTransform.StopReconcile(ReconcileState::Scale);
					}
					
					if (netTransform.IsReconciling(ReconcileState::Rotation))
					{
						float alpha = glm::clamp(replicationTimer / netTransform.ReconcileMaxTime, 0.0f, 1.0f);
						float interpolated = glm::mix(currentTransform.Rotation, predictedTransform.Rotation, alpha);
						transform.Rotation = interpolated;

						if (alpha == 1.0f)
							netTransform.StopReconcile(ReconcileState::Position);
					}
				}
				
				break;
			}
			}

			netTransform.ReplicationTimer += ts;
		}

		//_PT_CORE_TRACE("{:.9f}", testTimer.ElapsedMillis());

		if (m_Client && m_NetworkManager->IsNetworkTick())
		{
			Client_SendSequenceNumberMessage();
		}
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
}
