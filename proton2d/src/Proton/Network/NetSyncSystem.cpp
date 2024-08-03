#include "ptpch.h"
#include "Proton/Network/NetSyncSystem.h"
#include "Proton/Physics/PhysicsWorld.h"

#include <box2d/b2_world.h>
#include <glm/gtx/norm.hpp>

namespace proton {

	void NetSyncSystem::Update(Scene* scene, float ts)
	{
		PROFILE_FUNCTION();

		bool isPhysicsTick = scene->m_PhysicsTick;

		auto view = scene->m_Registry.view<NetworkComponent, TransformComponent, VelocityComponent>();
		for (auto e : view)
		{
			Entity entity(e, scene);
			auto [net, transform, velocity] = view.get<NetworkComponent, TransformComponent, VelocityComponent>(e);
			
			auto& previous = net.PreviousTransform;
			auto& current = net.CurrentTransform;
			auto& syncState = net.SyncState;
			auto& syncParams = net.SyncParams;

			if (syncParams.SyncMethod == NetSyncMethod::None)
				continue;

			//if (!net.TransformInitialized)
			//{
			//	transform.LocalPosition = current.Position;
			//	velocity.LinearVelocity = current.LinearVelocity;
			//	velocity.AngularVelocity = current.AngularVelocity;
			//	transform.Rotation = current.Rotation;
			//	net.TransformInitialized = true;
			//	continue;
			//}

			if (velocity.LinearVelocity == glm::vec2{ 0.0f } && current.LinearVelocity == glm::vec2{ 0.0f })
				continue;

			bool rbSync = net.SyncParams.SyncMethod == NetSyncMethod::NetworkRigidbody;
			//if (glm::distance(current.Position, rbSync ? transform.WorldPosition : transform.LocalPosition) < 0.0001f)
			//	continue;

			// Elapsed time from previous state to current (last packet update)
			float elapsed = syncState.PacketTimer.Elapsed();

			// Synchronize with Server State
			switch (syncParams.SyncMethod)
			{
			case NetSyncMethod::Interpolate:
			{
				if (elapsed < syncState.PacketDelay)
				{
					// Interpolate
					float alpha = glm::clamp(elapsed / syncState.PacketDelay, 0.0f, 1.0f);

					entity.SetWorldPosition(glm::mix(previous.Position, current.Position, alpha));
					velocity.LinearVelocity = glm::mix(previous.LinearVelocity, current.LinearVelocity, alpha);
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
				if (elapsed < syncParams.ExtrapolationLimit)
				{
					if (net.SyncState.NewPacket)
						transform.WorldPosition = current.Position;

					glm::vec3 newPosition = {
						transform.WorldPosition.x + velocity.LinearVelocity.x * ts,
						transform.WorldPosition.y + velocity.LinearVelocity.y * ts,
						transform.WorldPosition.z
					};
					entity.SetWorldPosition(newPosition);
					transform.Rotation += velocity.AngularVelocity * ts;
				}
				break;
			}
			case NetSyncMethod::NetworkRigidbody:
			{
				if (!scene->m_PhysicsTick)
					break;

				if (!entity.HasComponent<RigidbodyComponent>())
					break;

				auto& rb = entity.GetComponent<RigidbodyComponent>();
				b2Body* body = rb.RuntimeBody;

				if (!body || syncParams.SyncMethod != NetSyncMethod::NetworkRigidbody)
					break;

				glm::vec2 diff = current.LinearVelocity - previous.LinearVelocity;
				glm::vec2 estimatedVel = {
					current.LinearVelocity.x + (previous.LinearVelocity.x == 0.0f ? 0.0f : diff.x),
					current.LinearVelocity.y + (previous.LinearVelocity.y == 0.0f ? 0.0f : diff.y)
				};
				float multiplier = glm::min(syncState.PacketDelay, 1.0f / 16.0f);

				if (syncState.NewPacket)
				{
					syncState.ExtrapolatedPoint = {
						current.Position.x + current.LinearVelocity.x * multiplier,
						current.Position.y + current.LinearVelocity.y * multiplier,
						current.Position.z
					};
				}

				if (!syncState.ReconcileStarted && syncState.ReconcileCooldownTimer.Elapsed() > syncParams.ReconcileCooldownTime)
				{
					syncState.Error = glm::length2(syncState.ExtrapolatedPoint - transform.WorldPosition);

					float distance = glm::distance(
						glm::vec2{ transform.WorldPosition.x, transform.WorldPosition.y },
						glm::vec2{ syncState.ExtrapolatedPoint.x, syncState.ExtrapolatedPoint.y }
					);

					if (syncState.Error >= syncParams.ReconcileThreshold)
					{
						if (entity.GetTag() == "Player")
							_PT_CORE_TRACE("Reconcile: len2={:.3f}, dist={:.3}, mul={:.3f} vel={:.3f}", syncState.Error, distance, multiplier, current.LinearVelocity.x);

						syncState.ReconcileStarted = true;
						syncState.ReconcileTimer.Reset();
						syncState.ReconcileCooldownTimer.Reset();
						
						body->SetGravityScale(0.0f);
					}
				}

				if (syncState.ReconcileStarted)
				{
					body->SetTransform({ syncState.ExtrapolatedPoint.x, syncState.ExtrapolatedPoint.y }, current.Rotation);
					body->SetLinearVelocity({ estimatedVel.x, estimatedVel.y });

					float distanceError = glm::distance(
						glm::vec2{ transform.WorldPosition.x, transform.WorldPosition.y },
						glm::vec2{ syncState.ExtrapolatedPoint.x, syncState.ExtrapolatedPoint.y }
					);
					//float distanceError = glm::length2(syncState.ExtrapolatedPoint - transform.WorldPosition);

					if (distanceError < 0.005f)
					{
						syncState.ReconcileStarted = false;
						body->SetGravityScale(1.0f);
					}	
				}
				break;
			}
			}

			net.SyncState.NewPacket = false;
		}
	}

	void NetSyncSystem::UpdatePhysics(Scene* scene, float ts)
	{
		//	PROFILE_FUNCTION();

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
	}
}
