#include "ptpch.h"
#include "Proton/Network/Client/NetClientTransformSyncSystem.h"
#include "Proton/Physics/PhysicsWorld.h"

#include <box2d/b2_world.h>

namespace proton {

	float NetClientTransformSyncSystem::s_ExtrapolationTimeThreshold = 0.5f;

	void NetClientTransformSyncSystem::Update(Scene* scene, float ts)
	{
		PROFILE_FUNCTION();

		auto view = scene->m_Registry.view<NetworkComponent, TransformComponent, VelocityComponent>();
		for (auto e : view)
		{
			Entity entity(e, scene);
			auto [net, transform, velocity] = view.get<NetworkComponent, TransformComponent, VelocityComponent>(e);
			auto& netTransform = net.NetTransform;
			auto syncMethod = net.SyncMethod == NetTranformSyncMethod::Inherit ? scene->m_NetTranformSyncMethod : net.SyncMethod;

			if (syncMethod == NetTranformSyncMethod::None)
				continue;

			if (velocity.LinearVelocity == glm::vec2{ 0.0f } && netTransform.LinearVelocity == glm::vec2{ 0.0f })
				continue;

			float elapsed = netTransform.UpdateTimer.Elapsed();

			// Synchronize with Server State
			switch (syncMethod)
			{
			case NetTranformSyncMethod::Interpolate:
			{
				if (elapsed < netTransform.PacketDelay)
				{
					// Interpolate
					float alpha = glm::clamp(ts / netTransform.PacketDelay, 0.0f, 1.0f);

					transform.LocalPosition = glm::mix(transform.LocalPosition, netTransform.Position, alpha);
					velocity.LinearVelocity = glm::mix(velocity.LinearVelocity, netTransform.LinearVelocity, alpha);
					transform.Rotation = glm::mix(transform.Rotation, netTransform.Rotation, alpha);
				}
				// Extrapolate for some time if packet did not arrive at time
				else if (elapsed < scene->m_NetExtrapolationTimeThreshold)
				{
					transform.LocalPosition.x += velocity.LinearVelocity.x * ts;
					transform.LocalPosition.y += velocity.LinearVelocity.y * ts;
					transform.Rotation += velocity.AngularVelocity * ts;
				}
				break;
			}
			case NetTranformSyncMethod::Extrapolate:
			{
				if (elapsed < scene->m_NetExtrapolationTimeThreshold)
				{
					transform.LocalPosition.x += velocity.LinearVelocity.x * ts;
					transform.LocalPosition.y += velocity.LinearVelocity.y * ts;
					transform.Rotation += velocity.AngularVelocity * ts;
				}
				break;
			}
			case NetTranformSyncMethod::NetworkRigidbody:
			{
				// UpdatePhysics
				break;
			}
			}
		}
	}

	void NetClientTransformSyncSystem::UpdatePhysics(Scene* scene, float ts)
	{
		PROFILE_FUNCTION();

		auto view = scene->m_Registry.view<NetworkComponent, TransformComponent, VelocityComponent, RigidbodyComponent>();
		for (auto e : view)
		{
			Entity entity(e, scene);
			auto [net, transform, velocity, rb] = view.get<NetworkComponent, TransformComponent, VelocityComponent, RigidbodyComponent>(e);
			auto& netTransform = net.NetTransform;
			auto syncMethod = net.SyncMethod == NetTranformSyncMethod::Inherit ? scene->m_NetTranformSyncMethod : net.SyncMethod;

			if (syncMethod != NetTranformSyncMethod::NetworkRigidbody)
				continue;

			if (velocity.LinearVelocity == glm::vec2{ 0.0f } && netTransform.LinearVelocity == glm::vec2{ 0.0f })
				continue;

			if (!rb.RuntimeBody)
				continue;

			if (!net.NetTransform.ReconcileStarted)
			{
				float distance = glm::distance(
					glm::vec2{ transform.WorldPosition.x, transform.WorldPosition.y },
					glm::vec2{ netTransform.Position.x, netTransform.Position.y }
				);

				if (distance >= scene->m_NetReconcileThreshold)
				{
					if (entity.GetTag() == "Player")
					{
						_PT_CORE_TRACE("Start reconcile, distance={}", distance);
					}
					net.NetTransform.ReconcileStarted = true;
					net.NetTransform.ReconcileTimer.Reset();
					rb.RuntimeBody->SetGravityScale(0.0f);
					//rb.RuntimeBody->
				}
			}

			if (net.NetTransform.ReconcileStarted)
			{
				float elapsed = net.NetTransform.ReconcileTimer.Elapsed();
				if (elapsed <= scene->m_NetReconcileTime)
				{
					float alpha = glm::clamp((elapsed / scene->m_NetReconcileTime), 0.0f, 1.0f);

					glm::vec3 newPosition = glm::mix(netTransform.OldPosition, netTransform.Position, alpha);
					glm::vec2 newLinearVelocity = glm::mix(netTransform.OldLinearVelocity, netTransform.LinearVelocity, alpha);
					//float newRotation = glm::mix(transform.Rotation, netTransform.Rotation, alpha);
					//float newAngularVelocity = glm::mix(velocity.AngularVelocity, netTransform.AngularVelocity, alpha);

					rb.RuntimeBody->SetTransform({ newPosition.x, newPosition.y }, netTransform.Rotation);
					rb.RuntimeBody->SetLinearVelocity({ newLinearVelocity.x, newLinearVelocity.y });
					//body->SetAngularVelocity(newAngularVelocity);
				}
						
				if (elapsed >= scene->m_NetReconcileTime)
				{
					net.NetTransform.ReconcileStarted = false;
					rb.RuntimeBody->SetGravityScale(1.0f);
				}
			}

		}
	}
}
