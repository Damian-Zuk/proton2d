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
			const auto& lastAuthoritative = netTransform.LastAuthoritativeTransform;
			const auto& prevAuthoritative = netTransform.PrevAuthoritativeTransform;
			auto& replicationTimer = netTransform.ReplicationTimer;
			
			bool replicatedThisFrame = replicationTimer == 0.0f;

			// Get current transform values from TransformComponent
			const Transform current = Transform::Get(&transform);

			// Try to get Box2D rigidbody
			bool hasRigidbodySimulated = net.SimulateOnClient && entity.HasComponent<RigidbodyComponent>();
			b2Body* rigidbody = hasRigidbodySimulated ? entity.GetRuntimeBody() : nullptr;
			if (hasRigidbodySimulated && !rigidbody)
			{
				// Runtime body not created yet (entity has just been spawned)

				continue; // Skip this update tick
			}

			// Handle cull distance: put rigidbody to sleep when far away
			if (rigidbody)
			{
				if (Entity localPlayer = m_NetworkManager->GetLocalPlayerEntity())
				{
					auto& tc = localPlayer.GetTransform();
					auto& playerPos = rigidbody ? tc.WorldPosition : tc.LocalPosition;
					if (glm::distance(current.Position, { playerPos.x, playerPos.y }) > netTransform.CullDistance)
					{
						if (rigidbody->IsAwake())
						{
							rigidbody->SetAwake(false);
						}
					}
					else if (!rigidbody->IsAwake())
					{
						rigidbody->SetAwake(true);
					}
				}
			}

			// NetTransform member references
			const auto& reconcileThreshold = netTransform.ReconcileThreshold;
			const auto& reconcileTime = netTransform.ReconcileTime;
			const auto& reconcileCooldownTime = netTransform.ReconcileCooldownTime;
			auto& lastTickTransform = netTransform.LastTickTransform;
			auto& predicted = netTransform.PredictedTransform;
			auto& reconcileOffset = netTransform.ReconcileOffset;
			auto& interpolationTimer = netTransform.InterpolationTimer;
			auto& reconcileTimer = netTransform.ReconcileTimer;

			bool isLocalPlayer = m_NetworkManager->GetLocalPlayerEntity() == entity;

			// Handle transform sync for each method
			switch (netTransform.Method)
			{
			////////////////////////////////////////////////////////////////////////////////////////////////////
			case NetTransform::SyncMethod::None:
			{
				const auto& replicationFlags = netTransform.ReplicationFlags;

				if (EnumHasAnyFlags(replicationFlags, NetTransform::ReplicateComponents::Position))
				{
					if (!rigidbody)
					{
						entity.SetLocalPosition({ lastAuthoritative.Position.x, lastAuthoritative.Position.y, transform.LocalPosition.z });
					}
					else
					{
						// Use reconcile logic for rigidbody
						predicted.Position = { lastAuthoritative.Position.x, lastAuthoritative.Position.y };
						netTransform.StartReconcile(ReconcileComponents::Position);
					}
				}
				if (EnumHasAnyFlags(replicationFlags, NetTransform::ReplicateComponents::Scale))
				{
					transform.Scale = { lastAuthoritative.Scale.x, lastAuthoritative.Scale.y };
				}
				if (EnumHasAnyFlags(replicationFlags, NetTransform::ReplicateComponents::Rotation))
				{
					if (!rigidbody)
					{
						transform.Rotation = lastAuthoritative.Rotation;
					}
					else
					{
						// Use reconcile logic for rigidbody
						predicted.Rotation = lastAuthoritative.Rotation;
						netTransform.StartReconcile(ReconcileComponents::Rotation);
					}
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
				
				if (!rigidbody)
				{
					entity.SetLocalPosition(interpolatedPosition);
					transform.Scale = glm::mix(prevAuthoritative.Scale, lastAuthoritative.Scale, alpha);
					transform.Rotation = interpolatedRotation;
				}
				else
				{
					rigidbody->SetTransform({ interpolatedPosition.x, interpolatedPosition.y }, interpolatedRotation * (b2_pi / 180.0f));
				}
				predicted = lastAuthoritative;
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

				predicted.Position = {
					lastAuthoritative.Position.x + estimatedVelocity.x * lag,
					lastAuthoritative.Position.y + estimatedVelocity.y * lag
				};

				if (!netTransform.IsReconciling(ReconcileComponents::Position))
				{
					const float positionError = glm::distance(current.Position, predicted.Position);
					if (positionError >= reconcileThreshold)
						netTransform.StartReconcile(ReconcileComponents::Position);
				}

				// Rotation extrapolation
				const float currentAngularVelocity = rigidbody->GetAngularVelocity();
				const float serverAngularVelocity = (lastAuthoritative.Rotation - prevAuthoritative.Rotation) / m_NetworkManager->m_TickTime;
				const float estimatedAngularVelocity = (currentAngularVelocity + serverAngularVelocity) / 2.0f;

				predicted.Rotation = lastAuthoritative.Rotation + estimatedAngularVelocity * lag;
				
				if (!netTransform.IsReconciling(ReconcileComponents::Rotation))
				{
					constexpr float s_RotationReconcileThreshold = 5.0f;
					const float rotationError = glm::abs(current.Rotation - predicted.Rotation);
					if (rotationError > s_RotationReconcileThreshold)
						netTransform.StartReconcile(ReconcileComponents::Rotation);
				}

				break;
			}
			////////////////////////////////////////////////////////////////////////////////////////////////////
			case NetTransform::SyncMethod::Prediction:
			{
				auto& deltaBuffer = netTransform.DeltaBuffer;

				Transform thisTickDelta = current - lastTickTransform - reconcileOffset;
				lastTickTransform = current;
				reconcileOffset = Transform{};

				// Increment sequence number, store in deltaBuffer and send current sequence number to server
				if (thisTickDelta.IsNotZero())
				{
					deltaBuffer.push_back({ (uint16_t)(netTransform.CurrentSequenceNumber + 1), thisTickDelta });
				}
				
				if (m_NetworkManager->IsNetworkTick())
				{
					m_SequenceNumbersToSend.push_back({ entity.GetUUID(), (uint16_t)(netTransform.CurrentSequenceNumber + 1) });
					netTransform.CurrentSequenceNumber++;
				}

				// Remove deltas that happened before last authoritative transform 
				auto deltaIt = deltaBuffer.begin();
				while (deltaIt != deltaBuffer.end() && deltaIt->SequenceNumber <= netTransform.ServerSequenceNumber)
					deltaIt++;

				if (deltaIt != deltaBuffer.begin())
					deltaBuffer.erase(deltaBuffer.begin(), deltaIt);

				// Calculate predicted transform: apply deltas not processed by server yet
				predicted = lastAuthoritative;
				for (const auto& delta : deltaBuffer)
				{
					predicted.Position += delta.Value.Position;
					predicted.Scale += delta.Value.Scale;
					predicted.Rotation += delta.Value.Rotation;
				}

				// Check position error
				if (!netTransform.IsReconciling(ReconcileComponents::Position))
				{
					const float positionError = glm::distance(current.Position, predicted.Position);
					if (positionError > reconcileThreshold && reconcileTimer >= 0.0f)
					{
						//if (isLocalPlayer) _PT_CORE_TRACE("Start reconcile: error={}", positionError);
						netTransform.StartReconcile(ReconcileComponents::Position);
					}
				}

				// Check scale error
				//if (!netTransform.IsReconciling(ReconcileComponents::Scale))
				//{
				//	const float scaleError = glm::distance(current.Scale, predicted.Scale);
				//	if (scaleError > s_ScaleReconcileThreshold)
				//		netTransform.StartReconcile(ReconcileComponents::Scale);
				//}

				// Check rotation error
				//if (!netTransform.IsReconciling(ReconcileComponents::Rotation))
				//{
				//	const float rotationError = glm::abs(current.Rotation - predicted.Rotation);
				//	if (rotationError > s_RotationReconcileThreshold)
				//		netTransform.StartReconcile(ReconcileComponents::Rotation);
				//}

				break;
			}
			}
			// ---------------------------------------------------------------------------------------------------------------------

			// Handle Reconciliation
			if (rigidbody)
			{
				if (netTransform.IsReconciling(ReconcileComponents::Position))
				{
					reconcileTimer += ts;

					float alpha = reconcileTime > 0.0f ? glm::clamp(reconcileTimer / reconcileTime, 0.0f, 1.0f) : 1.0f;
					
					if (alpha > 0.0f)
					{
						glm::vec2 interpolated = glm::mix(current.Position, predicted.Position, alpha);
						glm::vec2 offset = interpolated - current.Position;

						entity.SetRigidbodyTransform(interpolated, current.Rotation);

						if (alpha == 1.0f)
						{
							//if (isLocalPlayer) _PT_CORE_TRACE("___Stop reconcile___");
							netTransform.StopReconcile(ReconcileComponents::Position);
							//lastTickTransform.Position = interpolated;
						}

						reconcileOffset.Position = offset * netTransform.ReconcileOffsetFactor;
					}
					
				}
			}


#if 0
			// Handle reconcile max time limit
			if (reconcileTime > 0.0f && reconcileTimer > reconcileTime)
				netTransform.StopReconcile(ReconcileComponents::Position);

			// ------------------------------------------ Box2D Rigidbody Reconciliation ------------------------------------------
			if (rigidbody)
			{
				constexpr float positionErrorThreshold = 0.05f;
				constexpr float rotationErrorThreshold = 1.0f;

				// Reconcile position and rotation (single b2Body::SetTransform function call)
				if (netTransform.IsReconciling(ReconcileComponents::PositionAndRotation))
				{
					const float positionError = glm::distance(current.Position, predicted.Position);
					if (positionError < positionErrorThreshold)
						netTransform.StopReconcile(ReconcileComponents::Position);

					const float rotationError = glm::abs(current.Rotation - predicted.Rotation);
					if (rotationError < rotationErrorThreshold)
						netTransform.StopReconcile(ReconcileComponents::Rotation);

					entity.SetRigidbodyTransform(predicted.Position, predicted.Rotation);
					lastTickTransform = current;
					
					if (netTransform.Method == NetTransform::SyncMethod::None)
						rigidbody->SetLinearVelocity({ 0.0f, 0.0f }); // ignore gravity (on default method)
					
					reconcileTimer += ts;
				}
				// Reconcile position only
				else if (netTransform.IsReconciling(ReconcileComponents::Position))
				{
					const float positionError = glm::distance(current.Position, predicted.Position);
					if (positionError < positionErrorThreshold)
						netTransform.StopReconcile(ReconcileComponents::Position);

					entity.SetRigidbodyTransform(predicted.Position, current.Rotation);
					lastTickTransform.Position = current.Position;

					if (netTransform.Method == NetTransform::SyncMethod::None)
						rigidbody->SetLinearVelocity({ 0.0f, 0.0f }); // ignore gravity (on default method)
					
					reconcileTimer += ts;
				}
				// Reconcile rotation only
				else if (netTransform.IsReconciling(ReconcileComponents::Rotation))
				{
					const float rotationError = glm::abs(current.Rotation - predicted.Rotation);
					if (rotationError < rotationErrorThreshold)
						netTransform.StopReconcile(ReconcileComponents::Rotation);
					lastTickTransform.Rotation = current.Rotation;

					entity.SetRigidbodyTransform(current.Position, predicted.Rotation);
				}
			}
			// ----------------------------------------- TransformComponent Reconciliation -----------------------------------------
			else // (with interpolation for smoothness)
			{
				const float alpha = netTransform.ReconcileTime > 0.0f ? glm::clamp(interpolationTimer / netTransform.ReconcileTime, 0.0f, 1.0f) : 1.0f;

				if (netTransform.IsReconciling(ReconcileComponents::Position))
				{
					entity.SetLocalPosition(glm::mix(current.Position, predicted.Position, alpha));
					reconcileTimer += ts;

					if (alpha == 1.0f)
						netTransform.StopReconcile(ReconcileComponents::Position);
				}

				if (netTransform.IsReconciling(ReconcileComponents::Scale))
				{
					transform.Scale = glm::mix(current.Scale, predicted.Scale, alpha);

					if (alpha == 1.0f)
						netTransform.StopReconcile(ReconcileComponents::Scale);
				}

				if (netTransform.IsReconciling(ReconcileComponents::Rotation))
				{
					transform.Rotation = glm::mix(current.Rotation, predicted.Rotation, alpha);

					if (alpha == 1.0f)
						netTransform.StopReconcile(ReconcileComponents::Position);
				}
			}
			// ---------------------------------------------------------------------------------------------------------------------
#endif 
			// Increment timers
			replicationTimer += ts;
			interpolationTimer += ts;

			// Reconcile timer is on cooldown
			if (reconcileTimer < 0)
				reconcileTimer = glm::min(reconcileTimer + ts, 0.0f);
		}

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
