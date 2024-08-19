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
	using ReconcileFlags = NetTransform::ReconcileFlags;

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

		auto view = scene->GetAllEntitiesWith<NetworkComponent, TransformComponent>();
		for (auto _entity : view)
		{
			Entity entity(_entity, scene);
			auto [net, transform] = view.get<NetworkComponent, TransformComponent>(_entity);

			// Try to retrieve Box2D rigidbody (b2Body*)
			const bool hasRigidbodySimulated = net.SimulateOnClient && entity.HasComponent<RigidbodyComponent>();
			b2Body* rigidbody = entity.GetRuntimeBody();
			if (hasRigidbodySimulated && !rigidbody)
				continue; // Runtime body not created yet, skip replication tick

			// Get current transform values from TransformComponent
			const Transform currentTransform = Transform::Get(&transform);
			
			// NetTransform member references
			auto& netTransform = net.NetTransform;
			const auto& lastAuthorative = netTransform.LastAuthoritativeTransform;
			const auto& prevAuthorative = netTransform.PrevAuthoritativeTransform;
			const auto& replicationInterval = netTransform.ReplicationInterval;
			const auto& reconcileMaxTime = netTransform.ReconcileMaxTime;
			const auto& reconcileCooldownTime = netTransform.ReconcileCooldownTime;
			auto& predictedTransform = netTransform.PredictedTransform;
			auto& interpolationTimer = netTransform.InterpolationTimer;
			auto& replicationTimer = netTransform.ReplicationTimer;

			// replicationTimer is being reset by NetReplicator
			const bool replicatedThisFrame = replicationTimer == 0.0f;

			// Handle transform sync for each method
			switch (netTransform.Method)
			{
			////////////////////////////////////////////////////////////////////////////////////////////////////
			case NetTransform::SyncMethod::None:
			{
				if (!replicatedThisFrame)
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
				// Interpolate between two last authoritative transforms
				const float alpha = replicationInterval > 0.0f ? glm::clamp(interpolationTimer / replicationInterval, 0.0f, 1.0f) : 1.0f;
				const glm::vec2 interpolatedPosition = glm::mix(prevAuthorative.Position, lastAuthorative.Position, alpha);
				const float interpolatedRotation = glm::mix(prevAuthorative.Rotation, lastAuthorative.Rotation, alpha);
				
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
				predictedTransform = lastAuthorative;
				break;
			}
			////////////////////////////////////////////////////////////////////////////////////////////////////
			case NetTransform::SyncMethod::Extrapolation:
			{
				if (!hasRigidbodySimulated) // extrapolation supported only for rigidbodies
					break;

				// Temporary ping value: Server::s_FakeServerLag
				const float lag = Server::s_FakeServerLag / 1000.0f;

				const glm::vec2 currentVelocity = { rigidbody->GetLinearVelocity().x, rigidbody->GetLinearVelocity().y };
				const glm::vec2 serverVelocity = (lastAuthorative.Position - prevAuthorative.Position) / m_NetworkManager->m_TickTime;
				const glm::vec2 estimatedVelocity = (currentVelocity + serverVelocity) / 2.0f;

				predictedTransform.Position = {
					lastAuthorative.Position.x + estimatedVelocity.x * lag,
					lastAuthorative.Position.y + estimatedVelocity.y * lag
				};

				if (!netTransform.IsReconciling(ReconcileFlags::Position))
				{
					const float distance = glm::distance(currentTransform.Position, predictedTransform.Position);

					if (distance >= netTransform.ReconcileThreshold)
					{
						_PT_CORE_TRACE("Reconcile pos ({}): error={}, lag={:.3f}, predicted={}, serverVel={}, estVel={}",
							entity.GetTag(), distance, lag, predictedTransform.Position, serverVelocity, estimatedVelocity);

						netTransform.StartReconcile(ReconcileFlags::Position);
					}
				}

				const float currentAngularVelocity = rigidbody->GetAngularVelocity();
				const float serverAngularVelocity = (lastAuthorative.Rotation - prevAuthorative.Rotation) / m_NetworkManager->m_TickTime;
				const float estimatedAngularVelocity = (currentAngularVelocity + serverAngularVelocity) / 2.0f;

				predictedTransform.Rotation = lastAuthorative.Rotation + estimatedAngularVelocity * lag;
				
				if (!netTransform.IsReconciling(ReconcileFlags::Rotation))
				{
					constexpr float rotationReconcileThreshold = 5.0f;
					const float rotationError = glm::abs(currentTransform.Rotation - predictedTransform.Rotation);
					if (rotationError > rotationReconcileThreshold)
					{
						_PT_CORE_TRACE("Reconcile rot ({}): lag={:.3f}, predicted={}", entity.GetTag(), lag, predictedTransform.Rotation);
						netTransform.StartReconcile(ReconcileFlags::Rotation);
					}
				}

				break;
			}
			////////////////////////////////////////////////////////////////////////////////////////////////////
			case NetTransform::SyncMethod::Prediction:
			{
				// Get current transform values and calculate last tick delta (world space if uses physics body)
				auto& deltaBuffer = netTransform.DeltaBuffer;
				
				// Push current delta and sequence number to the buffer
				if (m_NetworkManager->IsNetworkTick())
				{
					// Calculate this tick delta
					Transform thisTickDelta = currentTransform - netTransform.LastTickTransform;
					
					// Update last tick transform
					netTransform.LastTickTransform = currentTransform;

					// Ignore delta if reconciling
					if (netTransform.IsReconciling(ReconcileFlags::Position))
						thisTickDelta.Position = glm::vec2{ 0.0f, 0.0f };

					if (netTransform.IsReconciling(ReconcileFlags::Scale))
						thisTickDelta.Scale = glm::vec2{ 0.0f, 0.0f };

					if (netTransform.IsReconciling(ReconcileFlags::Rotation))
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

				// Recalculate predicted transform and check if reconcilation needed
				if (replicatedThisFrame)
				{
					// Remove deltas that happened before last authoritative transform 
					auto deltaIt = deltaBuffer.begin();
					while (deltaIt != deltaBuffer.end() && deltaIt->SequenceNumber <= netTransform.ServerSequenceNumber)
						deltaIt++;

					if (deltaIt != deltaBuffer.begin())
						deltaBuffer.erase(deltaBuffer.begin(), deltaIt);

					// Apply deltas not processed by server yet
					predictedTransform = lastAuthorative;
					for (const auto& delta : deltaBuffer)
					{
						predictedTransform.Position += delta.Value.Position;
						predictedTransform.Scale += delta.Value.Scale;
						predictedTransform.Rotation += delta.Value.Rotation;
					}

					// Check position error
					const float positionError = glm::distance(currentTransform.Position, predictedTransform.Position);
					if (positionError > netTransform.ReconcileThreshold)
						netTransform.StartReconcile(ReconcileFlags::Position);

					// Check scale error
					const float scaleError = glm::distance(currentTransform.Scale, predictedTransform.Scale);
					if (scaleError > netTransform.ReconcileThreshold)
						netTransform.StartReconcile(ReconcileFlags::Scale);

					// Check rotation error
					constexpr float rotationReconcileThreshold = 5.0f;
					const float rotationError = glm::abs(currentTransform.Rotation - predictedTransform.Rotation);
					if (rotationError > rotationReconcileThreshold)
						netTransform.StartReconcile(ReconcileFlags::Rotation);
				}

				break;
			}
			}

			// -------------------------------------------------- Transform reconcilation ----------------------------------------------------

			// Reconcilation for physics body (scale not implemented yet)
			if (rigidbody != nullptr)
			{
				constexpr float positionErrorThreshold = 0.01f;
				constexpr float rotationErrorThreshold = 1.0f;

				// Reconcile position and rotation
				if (netTransform.IsReconciling(ReconcileFlags::PositionAndRotation))
				{
					const float positionError = glm::distance(currentTransform.Position, predictedTransform.Position);
					if (positionError < positionErrorThreshold)
						netTransform.StopReconcile(ReconcileFlags::Position);

					const float rotationError = glm::abs(currentTransform.Rotation - predictedTransform.Rotation);
					if (rotationError < rotationErrorThreshold)
						netTransform.StopReconcile(ReconcileFlags::Rotation);

					rigidbody->SetTransform({ predictedTransform.Position.x, predictedTransform.Position.y }, predictedTransform.Rotation * (b2_pi / 180.0f));
				}
				// Reconcile position only
				else if (netTransform.IsReconciling(ReconcileFlags::Position))
				{
					const float positionError = glm::distance(currentTransform.Position, predictedTransform.Position);
					if (positionError < positionErrorThreshold)
						netTransform.StopReconcile(ReconcileFlags::Position);

					rigidbody->SetTransform({ predictedTransform.Position.x, predictedTransform.Position.y }, rigidbody->GetAngle());
				}
				// Reconcile rotation only
				else if (netTransform.IsReconciling(ReconcileFlags::Rotation))
				{
					const float rotationError = glm::abs(currentTransform.Rotation - predictedTransform.Rotation);
					if (rotationError < rotationErrorThreshold)
						netTransform.StopReconcile(ReconcileFlags::Rotation);

					rigidbody->SetTransform({ transform.WorldPosition.x, transform.WorldPosition.y }, predictedTransform.Rotation * (b2_pi / 180.0f));
				}
			}
			else // Reconcilation for TransformComponent (with interpolation for smoothness)
			{
				if (netTransform.IsReconciling(ReconcileFlags::Position))
				{
					const float alpha = netTransform.ReconcileMaxTime > 0.0f ? glm::clamp(interpolationTimer / netTransform.ReconcileMaxTime, 0.0f, 1.0f) : 1.0f;
					const glm::vec2 interpolated = glm::mix(currentTransform.Position, predictedTransform.Position, alpha);
					entity.SetLocalPosition(interpolated);

					if (alpha == 1.0f)
						netTransform.StopReconcile(ReconcileFlags::Position);
				}

				if (netTransform.IsReconciling(ReconcileFlags::Scale))
				{
					const float alpha = netTransform.ReconcileMaxTime > 0.0f ? glm::clamp(interpolationTimer / netTransform.ReconcileMaxTime, 0.0f, 1.0f) : 1.0f;
					const glm::vec2 interpolated = glm::mix(currentTransform.Scale, predictedTransform.Scale, alpha);
					transform.Scale = interpolated;

					if (alpha == 1.0f)
						netTransform.StopReconcile(ReconcileFlags::Scale);
				}

				if (netTransform.IsReconciling(ReconcileFlags::Rotation))
				{
					const float alpha = netTransform.ReconcileMaxTime > 0.0f ? glm::clamp(interpolationTimer / netTransform.ReconcileMaxTime, 0.0f, 1.0f) : 1.0f;
					const float interpolated = glm::mix(currentTransform.Rotation, predictedTransform.Rotation, alpha);
					transform.Rotation = interpolated;

					if (alpha == 1.0f)
						netTransform.StopReconcile(ReconcileFlags::Position);
				}
			}

			// Increment timers
			replicationTimer += ts;
			interpolationTimer += ts;
		}

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
