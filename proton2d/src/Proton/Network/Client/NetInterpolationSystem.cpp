#include "ptpch.h"
#include "Proton/Network/Client/NetInterpolationSystem.h"

namespace proton {

	void NetInterpolationSystem::Interpolate(Scene* scene, float ts)
	{

	}

	void NetInterpolationSystem::Extrapolate(Scene* scene, float ts)
	{
	}

	void NetInterpolationSystem::OnUpdate(Scene* scene, float ts)
	{
		auto view = scene->m_Registry.view<NetworkComponent, TransformComponent, VelocityComponent>();
		for (auto e : view)
		{
			auto [net, transform, velocity] = view.get<NetworkComponent, TransformComponent, VelocityComponent>(e);
			auto& data = net.InterpolationData;

			if (velocity.LinearVelocity == glm::vec2{ 0.0f } &&
				data.NextLinearVelocity == glm::vec2{ 0.0f })
				continue;

			float alpha = data.Delay;

			transform.LocalPosition = glm::mix(transform.LocalPosition, data.NextPosition, alpha);
			velocity.LinearVelocity = glm::mix(velocity.LinearVelocity, data.NextLinearVelocity, alpha);
		}
	}
}
