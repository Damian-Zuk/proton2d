#include <Proton.h>

using namespace proton;

#include "DynamicExtrapolation.h"

void DynamicExtrapolation::OnRegisterFields()
{
	REGISTER_FIELD(Float, InterpolationThreshold);
	REGISTER_FIELD(Float, SwitchCooldownTime);
	REGISTER_FIELD(Bool, AlphaVisualize);
}

bool DynamicExtrapolation::OnCreate()
{
	if (!IsNetModeClient())
		return true;

	if (HasComponent<BoxColliderComponent>())
	{
		auto& bc = GetComponent<BoxColliderComponent>();
		bc.ContactCallback.OnBegin = [&](PhysicsContact contact) { OnBeginPhysicsContact(contact); };
	}

	if (HasComponent<CircleColliderComponent>())
	{
		auto& cc = GetComponent<CircleColliderComponent>();
		cc.ContactCallback.OnBegin = [&](PhysicsContact contact) { OnBeginPhysicsContact(contact); };
	}

	if (AlphaVisualize)
		GetColor().a = 0.7f;

	return true;
}

void DynamicExtrapolation::OnBeginPhysicsContact(const PhysicsContact& contact)
{
	auto& netTransform = GetComponent<NetworkComponent>().NetTransform;
	if (netTransform.Method == NetSyncMethod::Extrapolation)
		return;

	Entity localPlayerEntity = GetNetworkManager()->GetLocalPlayerEntity();
	
	bool otherIsExtrapolated = contact.Other->HasScript<DynamicExtrapolation>()
		&& contact.Other->HasNetworkPrediction();

	if (otherIsExtrapolated || contact.Other->GetParent() == localPlayerEntity)
	{
		if (netTransform.Method == NetSyncMethod::Interpolation)
		{
			netTransform.Method = NetSyncMethod::Extrapolation;
			netTransform.LastTickTransform = NetTransform::Transform::Get(&GetTransform());
			m_PredictionTimer = 0.0f;
			if (AlphaVisualize) 
				GetColor().a = 1.0f;
		}
	}
}

void DynamicExtrapolation::OnUpdate(float ts)
{
	if (!IsNetModeClient())
		return;

	if (Entity localPlayer = GetNetworkManager()->GetLocalPlayerEntity())
	{
		auto& netTransform = GetComponent<NetworkComponent>().NetTransform;

		if (netTransform.Method == NetSyncMethod::Extrapolation)
		{
			m_PredictionTimer += ts;
			if (m_PredictionTimer > SwitchCooldownTime)
			{
				const NetTransform::Transform current = NetTransform::Transform::Get(&GetTransform());
				float distance = glm::distance(current.Position, netTransform.LastAuthoritativeTransform.Position);
			
				if (distance < InterpolationThreshold)
				{
					// Interpolate from current to last authoritative transform
					netTransform.Method = NetSyncMethod::Interpolation;
					netTransform.PrevAuthoritativeTransform = current;
					netTransform.InterpolationTimer = 0.0f;
					if (AlphaVisualize)
						GetColor().a = 0.7f;
				}
			}
		}
	}
}
