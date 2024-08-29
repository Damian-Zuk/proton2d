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
	using Components = NetTransform::Components;

	constexpr static float s_ScaleReconcileThreshold = 0.1f;
	constexpr static float s_RotationReconcileThreshold = 3.0f;

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
			
			// NetTransform references
			const auto& lastAuthoritative = netTransform.LastAuthoritativeTransform;
			const auto& prevAuthoritative = netTransform.PrevAuthoritativeTransform;
			const auto& reconcileThreshold = netTransform.ReconcileThreshold;
			const auto& reconcileTime = netTransform.ReconcileTime;
			const auto& reconcileCooldownTime = netTransform.ReconcileCooldownTime;
			const auto& teleportThreshold = netTransform.TeleportThreshold;
			const auto& replicationFlags = netTransform.ReplicationFlags;
			auto& replicationTimer = netTransform.ReplicationTimer;
			auto& currentSequenceNumber = netTransform.CurrentSequenceNumber;
			auto& hasNewDeltas = netTransform.HasNewDeltas;
			auto& lastTickTransform = netTransform.LastTickTransform;
			auto& predicted = netTransform.PredictedTransform;
			auto& reconcileOffset = netTransform.ReconcileOffset;
			auto& interpolationTimer = netTransform.InterpolationTimer;
			auto& reconcileTimer = netTransform.ReconcileTimer;
			
			// Get current transform values from TransformComponent
			const Transform current = Transform::Get(&transform);

			// Try to get Box2D rigidbody (nullptr if non physics entity)
			b2Body* rigidbody = entity.GetRuntimeBody();

			// Cull Distance: Put rigidbody to sleep when far away
			if (rigidbody)
			{
				if (Entity localPlayer = m_NetworkManager->GetLocalPlayerEntity())
				{
					const glm::vec3& target = localPlayer.GetTransform().WorldPosition;
					float distance = glm::distance(current.Position, { target.x, target.y });
					if (distance > netTransform.CullDistance)
					{
						if (rigidbody->IsAwake()) 
							rigidbody->SetAwake(false);
					}
					else if (!rigidbody->IsAwake())
						rigidbody->SetAwake(true);
				}
			}

			// Handle transform sync for each method
			switch (netTransform.Method)
			{
			////////////////////////////////////////////////////////////////////////////////////////////////////
			case NetTransform::SyncMethod::None:
			{
				// Check if replicated this frame
				if (replicationTimer > 0.0f) break;

				if (rigidbody) // physics body
				{
					if (netTransform.WasReplicated(Components::Position) && netTransform.WasReplicated(Components::Rotation))
						entity.SetRigidbodyTransform(lastAuthoritative.Position, lastAuthoritative.Rotation);
					
					else if (netTransform.WasReplicated(Components::Position))
						entity.SetRigidbodyTransform(lastAuthoritative.Position, current.Rotation);
					
					else if (netTransform.WasReplicated(Components::Rotation))
						entity.SetRigidbodyTransform(current.Position, lastAuthoritative.Rotation);
				}
				else // non physics entity
				{
					if (netTransform.WasReplicated(Components::Position))
						entity.SetLocalPosition({ lastAuthoritative.Position.x, lastAuthoritative.Position.y, transform.LocalPosition.z });

					if (netTransform.WasReplicated(Components::Scale))
						transform.Scale = { lastAuthoritative.Scale.x, lastAuthoritative.Scale.y };

					if (netTransform.WasReplicated(Components::Rotation))
						transform.Rotation = lastAuthoritative.Rotation;
				}

				break;
			}
			////////////////////////////////////////////////////////////////////////////////////////////////////
			case NetTransform::SyncMethod::Interpolation:
			{
				// Interpolate between two last authoritative transforms
				const float alpha = glm::clamp(interpolationTimer / m_NetworkManager->m_TickTime, 0.0f, 1.0f);
				const glm::vec2 interpolatedPosition = glm::mix(prevAuthoritative.Position, lastAuthoritative.Position, alpha);
				const float interpolatedRotation = glm::mix(prevAuthoritative.Rotation, lastAuthoritative.Rotation, alpha);
				
				if (rigidbody) // physics body
				{
					entity.SetRigidbodyTransform(interpolatedPosition, interpolatedRotation);
				}
				else // non physics entity
				{
					entity.SetLocalPosition(interpolatedPosition);
					transform.Rotation = interpolatedRotation;
					transform.Scale = glm::mix(prevAuthoritative.Scale, lastAuthoritative.Scale, alpha);
				}
				break;
			}
			////////////////////////////////////////////////////////////////////////////////////////////////////
			case NetTransform::SyncMethod::Extrapolation:
			{
				// Extrapolation supported only for rigidbodies
				if (!rigidbody) break;

				const float lag = Server::s_FakeServerLag / 1000.0f;
				
				// Rotation extrapolation
				const glm::vec2 serverVelocity = (lastAuthoritative.Position - prevAuthoritative.Position) / m_NetworkManager->m_TickTime;
				const glm::vec2 currentVelocity = { rigidbody->GetLinearVelocity().x, rigidbody->GetLinearVelocity().y };
				const glm::vec2 estimatedVelocity = (currentVelocity + serverVelocity) / 2.0f;

				predicted.Position = {
					lastAuthoritative.Position.x + estimatedVelocity.x * lag,
					lastAuthoritative.Position.y + estimatedVelocity.y * lag
				};

				if (!netTransform.IsReconciling(Components::Position))
				{
					const float positionError = glm::distance(current.Position, predicted.Position);
					if (positionError >= reconcileThreshold)
						netTransform.StartReconcile(Components::Position);
				}

				// Rotation extrapolation
				const float currentAngularVelocity = rigidbody->GetAngularVelocity();
				const float serverAngularVelocity = (lastAuthoritative.Rotation - prevAuthoritative.Rotation) / m_NetworkManager->m_TickTime;
				const float estimatedAngularVelocity = (currentAngularVelocity + serverAngularVelocity) / 2.0f;

				predicted.Rotation = lastAuthoritative.Rotation + estimatedAngularVelocity * lag;
				
				if (!netTransform.IsReconciling(Components::Rotation))
				{
					constexpr float s_RotationReconcileThreshold = 5.0f;
					const float rotationError = glm::abs(current.Rotation - predicted.Rotation);
					if (rotationError > s_RotationReconcileThreshold)
						netTransform.StartReconcile(Components::Rotation);
				}

				break;
			}
			////////////////////////////////////////////////////////////////////////////////////////////////////
			case NetTransform::SyncMethod::Prediction:
			{
				if (scene->IsPhysicsTick()) // TODO: Transform IsNetworkTick
				{
					auto& deltaBuffer = netTransform.DeltaBuffer;
					
					// Calculate this tick delta
					Transform thisTickDelta = current - lastTickTransform - reconcileOffset;
					reconcileOffset = Transform{};
					lastTickTransform = current;

					// If delta not zero, store entry in deltaBuffer
					if (thisTickDelta.IsNotZero())
					{
						// Current sequence number is incremented on network tick
						deltaBuffer.push_back({ currentSequenceNumber + 1, thisTickDelta });
						hasNewDeltas = true;
					}

					// Remove deltas that happened before the last authoritative transform 
					auto deltaIt = deltaBuffer.begin();
					while (deltaIt != deltaBuffer.end() && deltaIt->SequenceNumber <= netTransform.ServerSequenceNumber)
						deltaIt++;

					if (deltaIt != deltaBuffer.begin())
						deltaBuffer.erase(deltaBuffer.begin(), deltaIt);
					
					// If not moved and not received replication for some time, clear the delta buffer 
					if (!hasNewDeltas && replicationTimer > m_NetworkManager->m_TickTime * 10.0f)
						deltaBuffer.clear();

					// Calculate predicted transform: apply deltas not processed by server yet
					predicted = lastAuthoritative;
					for (const auto& delta : deltaBuffer)
					{
						predicted.Position += delta.Value.Position;
						predicted.Scale += delta.Value.Scale;
						predicted.Rotation += delta.Value.Rotation;
					}

					// Check position error
					const float positionError = glm::distance(current.Position, predicted.Position);
					const float scaleError = glm::distance(current.Scale, predicted.Scale);
					const float rotationError = glm::abs(current.Rotation - predicted.Rotation);
					
					// Handle position error
					if (positionError > reconcileThreshold && reconcileTimer >= 0.0f)
					{
						if (positionError >= teleportThreshold)
						{
							// Teleport entity if error exceeds threshold
							if (rigidbody)
								entity.SetRigidbodyTransform(predicted.Position, current.Rotation);
							else
								entity.SetLocalPosition(predicted.Position);
							
							netTransform.StopReconcile(Components::Position);
							lastTickTransform = predicted;
						}
						else
							netTransform.StartReconcile(Components::Position);
					}

					// Handle scale error
					if (scaleError > s_ScaleReconcileThreshold)
						netTransform.StartReconcile(Components::Scale);

					// Handle rotation error
					if (rotationError > s_RotationReconcileThreshold)
						netTransform.StartReconcile(Components::Rotation);
				}

				// If network tick and buffer has new deltas, increment and send sequence nummber
				if (hasNewDeltas && m_NetworkManager->IsNetworkTick())
				{
					m_SequenceNumbersToSend.push_back({ entity.GetUUID(), currentSequenceNumber });
					currentSequenceNumber++;
					hasNewDeltas = false;
				}

				break;
			}
			}

			// Thresholds below which reconciliation is stopped
			constexpr float positionErrorThreshold = 0.01f;
			constexpr float scaleErrorThreshold = 0.01f;
			constexpr float rotationErrorThreshold = 0.1f;

			// Physics body reconciliation
			if (rigidbody && scene->IsPhysicsTick())
			{
				// Position and rotation (for single Entity::SetRigidbodyTransform function call)
				if (netTransform.IsReconciling(Components::PositionAndRotation))
				{
					float alpha = reconcileTime > 0.0f ? glm::clamp(reconcileTimer / reconcileTime, 0.0f, 1.0f) : 1.0f;
					if (alpha > 0.0f)
					{
						glm::vec2 interpolatedPos = glm::mix(current.Position, predicted.Position, alpha);
						float interpolatedRot = glm::mix(current.Rotation, predicted.Rotation, alpha);

						reconcileOffset.Position = interpolatedPos - current.Position;
						reconcileOffset.Rotation = interpolatedRot - current.Rotation;
						entity.SetRigidbodyTransform(interpolatedPos, interpolatedRot);

						const float positionError = glm::distance(interpolatedPos, predicted.Position);
						if (positionError < positionErrorThreshold)
							netTransform.StopReconcile(Components::Position);

						const float rotationError = glm::abs(interpolatedRot - predicted.Rotation);
						if (rotationError < rotationErrorThreshold)
							netTransform.StopReconcile(Components::Rotation);
					}
					reconcileTimer += ts;
				}
				// Position
				else if (netTransform.IsReconciling(Components::Position))
				{
					float alpha = reconcileTime > 0.0f ? glm::clamp(reconcileTimer / reconcileTime, 0.0f, 1.0f) : 1.0f;
					if (alpha > 0.0f)
					{
						glm::vec2 interpolatedPos = glm::mix(current.Position, predicted.Position, alpha);

						reconcileOffset.Position = interpolatedPos - current.Position;
						entity.SetRigidbodyTransform(interpolatedPos, current.Rotation);

						const float positionError = glm::distance(interpolatedPos, predicted.Position);
						if (positionError < positionErrorThreshold)
							netTransform.StopReconcile(Components::Position);
					}
					reconcileTimer += ts;
				}
				// Rotation
				else if (netTransform.IsReconciling(Components::Rotation))
				{
					float alpha = reconcileTime > 0.0f ? glm::clamp(reconcileTimer / reconcileTime, 0.0f, 1.0f) : 1.0f;
					if (alpha > 0.0f)
					{
						float interpolatedRot = glm::mix(current.Rotation, predicted.Rotation, alpha);

						reconcileOffset.Rotation = interpolatedRot - current.Rotation;
						entity.SetRigidbodyTransform(current.Position, interpolatedRot);

						const float rotationError = glm::abs(interpolatedRot - predicted.Rotation);
						if (rotationError < rotationErrorThreshold)
							netTransform.StopReconcile(Components::Rotation);
					}
					reconcileTimer += ts;
				}
			}

			// Non physics entity reconciliation
			if (!rigidbody)
			{
				const float alpha = netTransform.ReconcileTime > 0.0f ? glm::clamp(reconcileTimer / netTransform.ReconcileTime, 0.0f, 1.0f) : 1.0f;

				// Position
				if (netTransform.IsReconciling(Components::Position))
				{
					if (alpha > 0.0f)
					{
						glm::vec2 interpolatedPos = glm::mix(current.Position, predicted.Position, alpha);
						entity.SetLocalPosition(interpolatedPos);

						const float positionError = glm::distance(interpolatedPos, predicted.Position);
						if (positionError < positionErrorThreshold)
							netTransform.StopReconcile(Components::Position);
					}
					reconcileTimer += ts;
				}

				// Scale
				if (netTransform.IsReconciling(Components::Scale))
				{
					if (alpha > 0.0f)
					{
						transform.Scale = glm::mix(current.Scale, predicted.Scale, alpha);
						
						const float scaleError = glm::distance(transform.Scale, predicted.Scale);
						if (scaleError < scaleErrorThreshold)
							netTransform.StopReconcile(Components::Scale);
					}
				}

				// Rotation
				if (netTransform.IsReconciling(Components::Rotation))
				{
					if (alpha > 0.0f)
					{
						transform.Rotation = glm::mix(current.Rotation, predicted.Rotation, alpha);

						const float rotationError = glm::abs(transform.Rotation - predicted.Rotation);
						if (rotationError > rotationErrorThreshold)
							netTransform.StopReconcile(Components::Rotation);
					}
				}
			}

			// Increment timers
			replicationTimer += ts;
			interpolationTimer += ts;

			// Reconcile timer on cooldown
			if (reconcileTimer < 0.0f)
				reconcileTimer = glm::min(reconcileTimer + ts, 0.0f);
		}

		// Send sequence numbers to server on network tick
		if (m_Client && m_NetworkManager->IsNetworkTick())
			Client_SendSequenceNumberMessage();
	}

	void NetTransformSystem::Client_SendSequenceNumberMessage()
	{
		PROFILE_FUNCTION();

		if (m_SequenceNumbersToSend.empty())
			return;

		NetworkStreamWriter stream(m_Client->m_ScratchBuffer);
		MessageEntitySequence header;
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

		MessageEntitySequence header;
		stream.ReadRaw(header);

		// Get active scene based on client entity
		auto clientEntityIt = m_Server->m_ClientToEntityMap.find(clientID);
		Scene* scene = clientEntityIt != m_Server->m_ClientToEntityMap.end()
			? clientEntityIt->second.GetScene() : m_Server->m_GameInstance->GetActiveScene();

		for (uint32_t i = 0; i < header.EntityCount; i++)
		{
			MessageEntitySequence::PayloadItem item;
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
