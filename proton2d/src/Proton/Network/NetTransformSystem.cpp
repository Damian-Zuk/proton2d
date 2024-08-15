#include "ptpch.h"
#include "Proton/Network/NetTransformSystem.h"
#include "Proton/Network/NetworkManager.h"
#include "Proton/Network/Client.h"
#include "Proton/Network/Server.h"
#include "Proton/Physics/PhysicsWorld.h"

#include <box2d/b2_world.h>
#include <glm/gtx/norm.hpp>

namespace proton {

	void NetTransformSystem::Update(Scene* scene, float ts)
	{
		PROFILE_FUNCTION();

		bool isPhysicsTick = scene->m_PhysicsTick;

		NetworkManager* netManager = scene->GetNetworkManager();
		if (netManager->IsNetModeClient() && !netManager->m_ClientGameStateInitialized)
			return;

		auto view = scene->m_Registry.view<NetworkComponent, TransformComponent, VelocityComponent>();
		for (auto e : view)
		{
			Entity entity(e, scene);
			auto [net, transform, velocity] = view.get<NetworkComponent, TransformComponent, VelocityComponent>(e);
			
			auto& netTransform = net.NetTransform;
			auto& previous = netTransform.PreviousTransform;
			auto& current = netTransform.CurrentTransform;
			auto& syncState = netTransform.SyncState;
			auto& syncParams = netTransform.SyncParams;

			//if (velocity.LinearVelocity == glm::vec2{ 0.0f } && current.LinearVelocity == glm::vec2{ 0.0f })
			//	continue;

			bool rbSync = syncParams.SyncMethod == NetSyncMethod::NetworkRigidbody;
			//if (glm::distance(current.Position, rbSync ? transform.WorldPosition : transform.LocalPosition) < 0.0001f)
			//	continue;

			// Elapsed time from previous state to current (last packet update)
			float elapsed = syncState.ReplicationTimer.Elapsed();

			// Synchronize with Server State
			switch (syncParams.SyncMethod)
			{
			case NetSyncMethod::None:
			{
				if (syncState.NewUpdate)
				{
					// Immediate set
					if (EnumHasAnyFlags(net.NetTransform.RepFlags, NetTransform::ReplicationFlags::Position))
					{
						entity.SetLocalPosition({ current.Position.x, current.Position.y, transform.LocalPosition.z });
					}
					if (EnumHasAnyFlags(net.NetTransform.RepFlags, NetTransform::ReplicationFlags::Rotation))
					{
						transform.Rotation = current.Rotation;
					}
				}
				break;
			}
			case NetSyncMethod::Interpolate:
			{
				if (elapsed < syncState.PacketDelay)
				{
					// Interpolate
					float alpha = glm::clamp(elapsed / syncState.PacketDelay, 0.0f, 1.0f);

					glm::vec2 interpolated = glm::mix(previous.Position, current.Position, alpha);
					entity.SetLocalPosition({interpolated.x, interpolated.y, transform.LocalPosition.z});
					//velocity.LinearVelocity = glm::mix(previous.LinearVelocity, current.LinearVelocity, alpha);
					transform.Rotation = glm::mix(previous.Rotation, current.Rotation, alpha);
				}
				// Extrapolate for some time if packet did not arrive at time
				else if (elapsed < syncParams.ExtrapolationLimit)
				{
					transform.LocalPosition.x += velocity.LinearVelocity.x * ts;
					transform.LocalPosition.y += velocity.LinearVelocity.y * ts;
					transform.Rotation += velocity.AngularVelocity * ts;
				}
				break;
			}
			case NetSyncMethod::Extrapolate:
			{
#if 0 
				if (elapsed < syncParams.ExtrapolationLimit)
				{
					if (syncState.NewPacket)
						transform.WorldPosition = current.Position;

					glm::vec3 newPosition = {
						transform.WorldPosition.x + velocity.LinearVelocity.x * ts,
						transform.WorldPosition.y + velocity.LinearVelocity.y * ts,
						transform.WorldPosition.z
					};
					entity.SetWorldPosition(newPosition);
					transform.Rotation += velocity.AngularVelocity * ts;
				}
#endif
				break;
			}
			case NetSyncMethod::NetworkRigidbody:
			{
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
						syncState.ReconcileTimer.Reset();
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
			}
			}

			syncState.NewUpdate = false;
		}
	}

	void NetTransformSystem::UpdatePhysics(Scene* scene, float ts)
	{
		//	PROFILE_FUNCTION();
#if 0
		auto view = scene->m_Registry.view<NetworkComponent, TransformComponent, VelocityComponent, RigidbodyComponent>();
		for (auto e : view)
		{
			Entity entity(e, scene);
			auto [net, transform, velocity, rb] = view.get<NetworkComponent, TransformComponent, VelocityComponent, RigidbodyComponent>(e);
				
			auto& previous = net.PreviousTransform;
			auto& current = net.CurrentTransform;
			auto& syncState = net.SyncState;
			auto& syncParams = net.SyncParams;	

			b2Body* body = rb.RuntimeBody;
			if (!body)
				continue;

			//body->SetTransform({ current.Position.x, current.Position.y }, current.Rotation);

		}
#endif
	}
}
