#include "ptpch.h"
#include "Proton/Network/Client/NetInterpolationSystem.h"

namespace proton {

	void NetInterpolationSystem::InterpolateAll(Scene* scene, float ts)
	{
		auto view = scene->m_Registry.view<NetworkComponent, TransformComponent, VelocityComponent>();
		for (auto entity : view)
		{
			auto [net, transform, velocity] = view.get<NetworkComponent, TransformComponent, VelocityComponent>(entity);
			auto& data = net.InterpolationData;

			if (velocity.LinearVelocity == glm::vec2{ 0.0f } && data.NextLinearVelocity == glm::vec2{ 0.0f })
				continue;

			if (data.UpdateTimer.Elapsed() < data.PacketDelay)
			{
				// Interpolation
				float alpha = glm::clamp(ts / data.PacketDelay, 0.0f, 1.0f);

				transform.LocalPosition = glm::mix(transform.LocalPosition, data.NextPosition, alpha);
				velocity.LinearVelocity = glm::mix(velocity.LinearVelocity, data.NextLinearVelocity, alpha);
			}
			else
			{
				// Extrapolation
				transform.LocalPosition.x += velocity.LinearVelocity.x * ts;
				transform.LocalPosition.y += velocity.LinearVelocity.y * ts;
			}
		}
	}
}
