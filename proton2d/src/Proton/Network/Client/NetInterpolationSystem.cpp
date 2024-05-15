#include "ptpch.h"
#include "Proton/Network/Client/NetInterpolationSystem.h"

namespace proton {

	float NetInterpolationSystem::s_ExtrapolationTimeThreshold = 0.5f;

	void NetInterpolationSystem::InterpolateAll(Scene* scene, float ts)
	{
		auto view = scene->m_Registry.view<NetworkComponent, TransformComponent, VelocityComponent>();
		for (auto entity : view)
		{
			auto [net, transform, velocity] = view.get<NetworkComponent, TransformComponent, VelocityComponent>(entity);
			auto& data = net.InterpolationData;

			if (velocity.LinearVelocity == glm::vec2{ 0.0f } && data.NextLinearVelocity == glm::vec2{ 0.0f })
				continue;

			float lastUpdateTime = data.UpdateTimer.Elapsed();

			if (lastUpdateTime < data.PacketDelay)
			{
				// Interpolate
				float alpha = glm::clamp(ts / data.PacketDelay, 0.0f, 1.0f);

				transform.LocalPosition = glm::mix(transform.LocalPosition, data.NextPosition, alpha);
				velocity.LinearVelocity = glm::mix(velocity.LinearVelocity, data.NextLinearVelocity, alpha);
				transform.Rotation = glm::mix(transform.Rotation, data.NextRotation, alpha);
			}
			else if (lastUpdateTime < s_ExtrapolationTimeThreshold)
			{
				// Extrapolate
				transform.LocalPosition.x += velocity.LinearVelocity.x * ts;
				transform.LocalPosition.y += velocity.LinearVelocity.y * ts;
				transform.Rotation += velocity.AngularVelocity * ts;
			}
		}
	}
}
