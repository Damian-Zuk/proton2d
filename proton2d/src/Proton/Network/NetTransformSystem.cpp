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
	using ReconcileComponents = NetTransform::ReconcileComponents;

	constexpr static float s_ScaleReconcileThreshold = 0.1f;
	constexpr static float s_RotationReconcileThreshold = 5.0f;

	NetTransformSystem::NetTransformSystem(Client* client)
		: m_Client(client), m_NetworkManager(client->m_NetworkManager)
	{
	}

	NetTransformSystem::NetTransformSystem(Server* server)
		: m_Server(server), m_NetworkManager(server->m_NetworkManager)
	{ 
	}

	void NetTransformSystem::Client_OnUpdate(Scene* scene, float ts)
	{
		PROFILE_FUNCTION();

		auto view = scene->GetAllEntitiesWith<NetworkComponent, TransformComponent>();
		for (auto _entity : view)
		{
			Entity entity(_entity, scene);
			auto [net, transform] = view.get<NetworkComponent, TransformComponent>(_entity);
			auto& netTransform = net.NetTransform;

			// Try to retrieve Box2D rigidbody (b2Body*)
			bool hasRigidbodySimulated = net.SimulateOnClient && entity.HasComponent<RigidbodyComponent>();
			
			b2Body* rigidbody = hasRigidbodySimulated ? entity.GetRuntimeBody() : nullptr;

			if (hasRigidbodySimulated && !rigidbody)
			{
				// Runtime body not created yet (entity has been just spawned)
				// Set authoritative transform values but skip this update tick
				SetAuthoritativeTransform(entity, &transform, netTransform, false);
				continue; 
			}

			// Get current transform values from TransformComponent
			const Transform currentTransform = Transform::Get(&transform);
			
			// NetTransform member references
			const auto& lastAuthoritative = netTransform.LastAuthoritativeTransform;
			const auto& prevAuthoritative = netTransform.PrevAuthoritativeTransform;
			const auto& reconcileThreshold = netTransform.ReconcileThreshold;
			const auto& reconcileMaxTime = netTransform.ReconcileMaxTime;
			const auto& reconcileCooldownTime = netTransform.ReconcileCooldownTime;
			auto& predictedTransform = netTransform.PredictedTransform;
			auto& interpolationTimer = netTransform.InterpolationTimer;
			auto& replicationTimer = netTransform.ReplicationTimer;
			auto& reconcileTimer = netTransform.ReconcileTimer;

			bool replicatedThisFrame = replicationTimer == 0.0f;

			// Handle transform sync for each method
			switch (netTransform.Method)
			{
			////////////////////////////////////////////////////////////////////////////////////////////////////
			case NetTransform::SyncMethod::None:
			{
				SetAuthoritativeTransform(entity, &transform, netTransform);
				break;
			}
			////////////////////////////////////////////////////////////////////////////////////////////////////
			case NetTransform::SyncMethod::Interpolation:
			{
				// Interpolate between two last authoritative transforms
				const float alpha = glm::clamp(interpolationTimer / m_NetworkManager->m_TickTime, 0.0f, 1.0f);
				const glm::vec2 interpolatedPosition = glm::mix(prevAuthoritative.Position, lastAuthoritative.Position, alpha);
				const float interpolatedRotation = glm::mix(prevAuthoritative.Rotation, lastAuthoritative.Rotation, alpha);
				
				if (rigidbody)
				{
					rigidbody->SetTransform({ interpolatedPosition.x, interpolatedPosition.y }, interpolatedRotation * (b2_pi / 180.0f));
				}
				else
				{
					entity.SetLocalPosition(interpolatedPosition);
					transform.Scale = glm::mix(prevAuthoritative.Scale, lastAuthoritative.Scale, alpha);
					transform.Rotation = interpolatedRotation;
				}
				predictedTransform = lastAuthoritative;
				break;
			}
			////////////////////////////////////////////////////////////////////////////////////////////////////
			case NetTransform::SyncMethod::Extrapolation:
			{
				if (!hasRigidbodySimulated) // extrapolation supported only for rigidbodies
					break;

				// Temporary ping value: Server::s_FakeServerLag
				// TODO: m_Client->GetCurrentLag()
				const float lag = Server::s_FakeServerLag / 1000.0f;

				// TODO: Use RTT instead of m_NetworkManager->m_TickTime
				const glm::vec2 serverVelocity = (lastAuthoritative.Position - prevAuthoritative.Position) / m_NetworkManager->m_TickTime;
				const glm::vec2 currentVelocity = { rigidbody->GetLinearVelocity().x, rigidbody->GetLinearVelocity().y };
				const glm::vec2 estimatedVelocity = (currentVelocity + serverVelocity) / 2.0f;

				predictedTransform.Position = {
					lastAuthoritative.Position.x + estimatedVelocity.x * lag,
					lastAuthoritative.Position.y + estimatedVelocity.y * lag
				};

				if (!netTransform.IsReconciling(ReconcileComponents::Position))
				{
					const float positionError = glm::distance(currentTransform.Position, predictedTransform.Position);
					if (positionError >= reconcileThreshold)
						netTransform.StartReconcile(ReconcileComponents::Position);
				}

				// Rotation extrapolation
				const float currentAngularVelocity = rigidbody->GetAngularVelocity();
				const float serverAngularVelocity = (lastAuthoritative.Rotation - prevAuthoritative.Rotation) / m_NetworkManager->m_TickTime;
				const float estimatedAngularVelocity = (currentAngularVelocity + serverAngularVelocity) / 2.0f;

				predictedTransform.Rotation = lastAuthoritative.Rotation + estimatedAngularVelocity * lag;
				
				if (!netTransform.IsReconciling(ReconcileComponents::Rotation))
				{
					constexpr float s_RotationReconcileThreshold = 5.0f;
					const float rotationError = glm::abs(currentTransform.Rotation - predictedTransform.Rotation);
					if (rotationError > s_RotationReconcileThreshold)
						netTransform.StartReconcile(ReconcileComponents::Rotation);
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
					if (netTransform.IsReconciling(ReconcileComponents::Position))
						thisTickDelta.Position = glm::vec2{ 0.0f, 0.0f };

					if (netTransform.IsReconciling(ReconcileComponents::Scale))
						thisTickDelta.Scale = glm::vec2{ 0.0f, 0.0f };

					if (netTransform.IsReconciling(ReconcileComponents::Rotation))
						thisTickDelta.Rotation = 0.0f;
					
					// Increment sequence number, store in deltaBuffer and send current sequence number to server
					if (thisTickDelta.IsNotZero())
					{
						netTransform.CurrentSequenceNumber++;
						deltaBuffer.push_back({ netTransform.CurrentSequenceNumber, thisTickDelta });
						m_SequenceNumbersToSend.push_back({ entity.GetUUID(), netTransform.CurrentSequenceNumber });
					}
				}

				// Calculate predicted transform and check if reconciliation needed
				if (replicatedThisFrame)
				{
					// Remove deltas that happened before last authoritative transform 
					auto deltaIt = deltaBuffer.begin();
					while (deltaIt != deltaBuffer.end() && deltaIt->SequenceNumber <= netTransform.ServerSequenceNumber)
						deltaIt++;

					if (deltaIt != deltaBuffer.begin())
						deltaBuffer.erase(deltaBuffer.begin(), deltaIt);

					// Calculate predicted transform: apply deltas not processed by server yet
					predictedTransform = lastAuthoritative;
					for (const auto& delta : deltaBuffer)
					{
						predictedTransform.Position += delta.Value.Position;
						predictedTransform.Scale += delta.Value.Scale;
						predictedTransform.Rotation += delta.Value.Rotation;
					}

					// Check position error
					if (!netTransform.IsReconciling(ReconcileComponents::Position))
					{
						const float positionError = glm::distance(currentTransform.Position, predictedTransform.Position);
						if (positionError > reconcileThreshold && reconcileTimer >= 0)
							netTransform.StartReconcile(ReconcileComponents::Position);
					}

					// Check scale error
					if (!netTransform.IsReconciling(ReconcileComponents::Scale))
					{
						const float scaleError = glm::distance(currentTransform.Scale, predictedTransform.Scale);
						if (scaleError > s_ScaleReconcileThreshold)
							netTransform.StartReconcile(ReconcileComponents::Scale);
					}

					// Check rotation error
					if (!netTransform.IsReconciling(ReconcileComponents::Rotation))
					{
						const float rotationError = glm::abs(currentTransform.Rotation - predictedTransform.Rotation);
						if (rotationError > s_RotationReconcileThreshold)
							netTransform.StartReconcile(ReconcileComponents::Rotation);
					}
				}

				break;
			}
			}
			// ---------------------------------------------------------------------------------------------------------------------

			// Handle reconcile max time limit
			if (reconcileTimer > reconcileMaxTime)
			{
				netTransform.StopReconcile(ReconcileComponents::Position);
			}

			// ------------------------------------------ Box2D Rigidbody Reconciliation ------------------------------------------
			if (rigidbody) // NOTE: (scale not implemented yet)
			{
				constexpr float positionErrorThreshold = 0.01f;
				constexpr float rotationErrorThreshold = 1.0f;

				// Reconcile position and rotation (together because of single b2Body::SetTransform function call)
				if (netTransform.IsReconciling(ReconcileComponents::PositionAndRotation))
				{
					const float positionError = glm::distance(currentTransform.Position, predictedTransform.Position);
					if (positionError < positionErrorThreshold)
						netTransform.StopReconcile(ReconcileComponents::Position);

					const float rotationError = glm::abs(currentTransform.Rotation - predictedTransform.Rotation);
					if (rotationError < rotationErrorThreshold)
						netTransform.StopReconcile(ReconcileComponents::Rotation);

					rigidbody->SetTransform({ predictedTransform.Position.x, predictedTransform.Position.y }, predictedTransform.Rotation * (b2_pi / 180.0f));
					reconcileTimer += ts;
				}
				// Reconcile position only
				else if (netTransform.IsReconciling(ReconcileComponents::Position))
				{
					const float positionError = glm::distance(currentTransform.Position, predictedTransform.Position);
					if (positionError < positionErrorThreshold)
						netTransform.StopReconcile(ReconcileComponents::Position);

					rigidbody->SetTransform({ predictedTransform.Position.x, predictedTransform.Position.y }, rigidbody->GetAngle());
					reconcileTimer += ts;
				}
				// Reconcile rotation only
				else if (netTransform.IsReconciling(ReconcileComponents::Rotation))
				{
					const float rotationError = glm::abs(currentTransform.Rotation - predictedTransform.Rotation);
					if (rotationError < rotationErrorThreshold)
						netTransform.StopReconcile(ReconcileComponents::Rotation);

					rigidbody->SetTransform({ transform.WorldPosition.x, transform.WorldPosition.y }, predictedTransform.Rotation * (b2_pi / 180.0f));
				}
			}
			// ----------------------------------------- TransformComponent Reconciliation -----------------------------------------
			else // (with interpolation for smoothness)
			{
				const float alpha = netTransform.ReconcileMaxTime > 0.0f ? glm::clamp(interpolationTimer / netTransform.ReconcileMaxTime, 0.0f, 1.0f) : 1.0f;

				if (netTransform.IsReconciling(ReconcileComponents::Position))
				{
					const glm::vec2 predicted = glm::mix(currentTransform.Position, predictedTransform.Position, alpha);
					entity.SetLocalPosition(predicted);
					reconcileTimer += ts;

					if (alpha == 1.0f)
						netTransform.StopReconcile(ReconcileComponents::Position);
				}

				if (netTransform.IsReconciling(ReconcileComponents::Scale))
				{
					const glm::vec2 predicted = glm::mix(currentTransform.Scale, predictedTransform.Scale, alpha);
					transform.Scale = predicted;

					if (alpha == 1.0f)
						netTransform.StopReconcile(ReconcileComponents::Scale);
				}

				if (netTransform.IsReconciling(ReconcileComponents::Rotation))
				{
					const float predicted = glm::mix(currentTransform.Rotation, predictedTransform.Rotation, alpha);
					transform.Rotation = predicted;

					if (alpha == 1.0f)
						netTransform.StopReconcile(ReconcileComponents::Position);
				}
			}
			// ---------------------------------------------------------------------------------------------------------------------

			// Increment timers
			replicationTimer += ts;
			interpolationTimer += ts;

			// Reconcile timer is on cooldown
			if (reconcileTimer < 0)
				reconcileTimer = glm::min(reconcileTimer + ts, 0.0f);
		}

		if (m_Client && m_NetworkManager->IsNetworkTick())
		{
			Client_SendSequenceNumberMessage();
		}
	}

	void NetTransformSystem::SetAuthoritativeTransform(Entity entity, TransformComponent* transform, const NetTransform& netTransform, bool localSpace)
	{
		bool replicatedThisFrame = netTransform.ReplicationTimer == 0.0f;
		if (!replicatedThisFrame)
			return;

		const auto& replicationFlags = netTransform.ReplicationFlags;
		const auto& lastAuthorative = netTransform.LastAuthoritativeTransform;

		if (EnumHasAnyFlags(replicationFlags, NetTransform::ReplicateComponents::Position))
		{
			if (localSpace)
			{
				entity.SetLocalPosition({ lastAuthorative.Position.x, lastAuthorative.Position.y, transform->LocalPosition.z });
			}
			else
			{
				entity.SetWorldPosition({ lastAuthorative.Position.x, lastAuthorative.Position.y, transform->LocalPosition.z });
			}
		}
		if (EnumHasAnyFlags(replicationFlags, NetTransform::ReplicateComponents::Scale))
		{
			transform->Scale = { lastAuthorative.Scale.x, lastAuthorative.Scale.y };
		}
		if (EnumHasAnyFlags(replicationFlags, NetTransform::ReplicateComponents::Rotation))
		{
			transform->Rotation = lastAuthorative.Rotation;
		}
	}

	void NetTransformSystem::Client_SendSequenceNumberMessage()
	{
		PROFILE_FUNCTION();

		if (m_SequenceNumbersToSend.empty())
			return;

		NetworkStreamWriter stream(m_Client->m_ScratchBuffer);
		NetMessageTransformSequence header;
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

		NetMessageTransformSequence header;
		stream.ReadRaw(header);

		// Get active scene based on client entity
		auto clientEntityIt = m_Server->m_ClientToEntityMap.find(clientID);
		Scene* scene = clientEntityIt != m_Server->m_ClientToEntityMap.end()
			? clientEntityIt->second.GetScene() : m_Server->m_GameInstance->GetActiveScene();

		for (uint32_t i = 0; i < header.EntityCount; i++)
		{
			NetMessageTransformSequence::PayloadItem item;
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
				auto& data = net.NetTransform.ServerDataMap[clientID];
				data.SequenceNumber = item.SequenceNumber;
			}
		}
	}
}
