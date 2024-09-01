#include <Proton.h>
using namespace proton;

#include "DynamicExtrapolation.h"

void DynamicExtrapolation::OnRegisterFields()
{
	REGISTER_FIELD(Float, m_Range);
}

void DynamicExtrapolation::OnUpdate(float ts)
{
	NetworkManager* netManager = GetNetworkManager();

	if (netManager->IsNetModeClient())
	{
		if (!m_LocalPlayer)
			m_LocalPlayer = netManager->GetLocalPlayerEntity();

		if (m_LocalPlayer)
		{
			m_CooldownTimer += ts;

			auto& transform = GetComponent<TransformComponent>();
			auto& net = GetComponent<NetworkComponent>();
			auto& method = net.NetTransform.Method;

			const NetTransform::Transform current = NetTransform::Transform::Get(&transform);

			const auto& playerPosition = m_LocalPlayer.GetTransform().WorldPosition;
			const auto& position = transform.WorldPosition;
			float distance = glm::distance(
				glm::vec2{ position.x, position.y },
				glm::vec2{ playerPosition.x, playerPosition.y }
			);

			if (distance > m_Range)
			{
				if (method == NetSyncMethod::Extrapolation && m_CooldownTimer >= m_Cooldown)
				{
					method = NetSyncMethod::Interpolation;
					m_CooldownTimer = 0.0f;
				}
			}
			else
			{
				if (method == NetSyncMethod::Interpolation && m_CooldownTimer >= m_Cooldown)
				{
					method = NetSyncMethod::Extrapolation;
					m_CooldownTimer = 0.0f;
				}
			}
		}

	}
}