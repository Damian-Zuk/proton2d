#include "ptpch.h"
#include "Proton/Network/Client/NetSyncSystem.h"
#include "Proton/Physics/PhysicsWorld.h"

#include <box2d/b2_world.h>

namespace proton {

	void NetSyncSystem::Update(Scene* scene, float ts)
	{
		PROFILE_FUNCTION();

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

			if (!net.TransformInitialized)
			{
				transform.LocalPosition = current.Position;
				velocity.LinearVelocity = current.LinearVelocity;
				velocity.AngularVelocity = current.AngularVelocity;
				transform.Rotation = current.Rotation;

				net.TransformInitialized = true;
				continue;
			}

			if (velocity.LinearVelocity == glm::vec2{ 0.0f } && current.LinearVelocity == glm::vec2{ 0.0f })
				continue;

			if (glm::distance(current.Position, previous.Position) < 0.0001f)
				continue;

			if (net.SyncParams.SyncMethod == NetSyncMethod::NetworkRigidbody)
			{
				if (glm::distance(current.Position, transform.WorldPosition) < 0.0001f)
					continue;
			}
			else
			{
				if (glm::distance(current.Position, transform.LocalPosition) < 0.0001f)
					continue;
			}

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
				// Handle in UpdatePhysics
				break;
			}
			}

			net.SyncState.NewPacket = false;
		}
	}

	void NetSyncSystem::UpdatePhysics(Scene* scene, float ts)
	{
		PROFILE_FUNCTION();

		auto view = scene->m_Registry.view<NetworkComponent, TransformComponent, VelocityComponent, RigidbodyComponent>();
		for (auto e : view)
		{
			Entity entity(e, scene);
			auto [net, transform, velocity, rb] = view.get<NetworkComponent, TransformComponent, VelocityComponent, RigidbodyComponent>(e);
			
			auto& previous = net.PreviousTransform;
			auto& current = net.CurrentTransform;
			auto& syncState = net.SyncState;
			auto& syncParams = net.SyncParams;

			if (syncParams.SyncMethod != NetSyncMethod::NetworkRigidbody)
				continue;

			if (velocity.LinearVelocity == glm::vec2{ 0.0f } && current.LinearVelocity == glm::vec2{ 0.0f })
				continue;

			if (!rb.RuntimeBody)
				continue;

			if (!syncState.ReconcileStarted)
			{
				float distance = glm::distance(
					glm::vec2{ transform.WorldPosition.x, transform.WorldPosition.y },
					glm::vec2{ current.Position.x, current.Position.y }
				);

				if (distance >= syncParams.ReconcileThreshold)
				{
					if (entity.GetTag() == "Player")
					{
						_PT_CORE_TRACE("Start reconcile, distance={}", distance);
					}
					syncState.ReconcileStarted = true;
					syncState.ReconcileTimer.Reset();
					rb.RuntimeBody->SetGravityScale(0.0f);
					//rb.RuntimeBody->
				}
			}

			if (syncState.ReconcileStarted)
			{
				float elapsed = syncState.ReconcileTimer.Elapsed();
				if (elapsed <= syncParams.ReconcileTime)
				{
					float alpha = glm::clamp((elapsed / syncParams.ReconcileTime), 0.0f, 1.0f);

					// wrong
					glm::vec3 newPosition = glm::mix(previous.Position, current.Position, alpha);
					glm::vec2 newLinearVelocity = glm::mix(previous.LinearVelocity, previous.LinearVelocity, alpha);
					//float newRotation = glm::mix(transform.Rotation, netTransform.Rotation, alpha);
					//float newAngularVelocity = glm::mix(velocity.AngularVelocity, netTransform.AngularVelocity, alpha);

					rb.RuntimeBody->SetTransform({ newPosition.x, newPosition.y }, current.Rotation);
					rb.RuntimeBody->SetLinearVelocity({ newLinearVelocity.x, newLinearVelocity.y });
					//body->SetAngularVelocity(newAngularVelocity);
				}
						
				if (elapsed >= syncParams.ReconcileTime)
				{
					syncState.ReconcileStarted = false;
					rb.RuntimeBody->SetGravityScale(1.0f);
				}
			}

		}
	}
}
