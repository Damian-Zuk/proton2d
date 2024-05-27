#include "ptpch.h"
#include "Proton/Network/Client/NetInterpolationSystem.h"

namespace proton {

	float NetInterpolationSystem::s_ExtrapolationTimeThreshold = 0.5f;

	void NetInterpolationSystem::InterpolateAll(Scene* scene, float ts)
	{
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

			float lastUpdateTime = netTransform.UpdateTimer.Elapsed();

			switch (syncMethod)
			{
			case NetTranformSyncMethod::Interpolate:
			{
				if (lastUpdateTime < netTransform.PacketDelay)
				{
					// Interpolate
					float alpha = glm::clamp(ts / netTransform.PacketDelay, 0.0f, 1.0f);

					transform.LocalPosition = glm::mix(transform.LocalPosition, netTransform.Position, alpha);
					velocity.LinearVelocity = glm::mix(velocity.LinearVelocity, netTransform.LinearVelocity, alpha);
					transform.Rotation = glm::mix(transform.Rotation, netTransform.Rotation, alpha);
				}
				// Extrapolate for some time if packet did not arrive at time
				else if (lastUpdateTime < s_ExtrapolationTimeThreshold)
				{
					transform.LocalPosition.x += velocity.LinearVelocity.x * ts;
					transform.LocalPosition.y += velocity.LinearVelocity.y * ts;
					transform.Rotation += velocity.AngularVelocity * ts;
				}
				break;
			}
			case NetTranformSyncMethod::Extrapolate:
			{
				if (lastUpdateTime < s_ExtrapolationTimeThreshold)
				{
					transform.LocalPosition.x += velocity.LinearVelocity.x * ts;
					transform.LocalPosition.y += velocity.LinearVelocity.y * ts;
					transform.Rotation += velocity.AngularVelocity * ts;
				}
				break;
			}
			case NetTranformSyncMethod::NetworkRigidbody:
			{
				//if (entity.HasComponent<RigidbodyComponent>())
				//{
				//	auto& rb = entity.GetComponent<RigidbodyComponent>();
				//	rb.RuntimeBody->SetTransform({netTransform.Position.x, netTransform.Position.y}, netTransform.Rotation);
				//}
				break;
			}
			}
		}
	}
}
