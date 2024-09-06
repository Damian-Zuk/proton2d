#include <Proton.h>
using namespace proton;

#include "NetDynamicPrediction.h"

void NetDynamicPrediction::OnRegisterFields()
{
	REGISTER_FIELD(Float, InterpolationThreshold);
	REGISTER_FIELD(Float, SwitchCooldown);
	REGISTER_FIELD(Bool, AlphaVisualize);
}

bool NetDynamicPrediction::OnCreate()
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

void NetDynamicPrediction::OnBeginPhysicsContact(const PhysicsContact& contact)
{
	auto& netTransform = GetComponent<NetworkComponent>().NetTransform;
	if (netTransform.Method == NetSyncMethod::Prediction)
		return;

	Entity localPlayerEntity = GetNetworkManager()->GetLocalPlayerEntity();

	bool otherHasDynamicPrediction = contact.Other->HasScript<NetDynamicPrediction>()
		&& contact.Other->GetNetSyncMethod() == NetSyncMethod::Prediction;

	if (otherHasDynamicPrediction || contact.Other->GetParent() == localPlayerEntity)
	{
		if (netTransform.Method == NetSyncMethod::Interpolation)
		{
			netTransform.Method = NetSyncMethod::Prediction;
			netTransform.LastTickTransform = NetTransform::Transform::Get(&GetTransform());
			m_PredictionTimer = 0.0f;
			if (AlphaVisualize)
				GetColor().a = 1.0f;
		}
	}
}

void NetDynamicPrediction::OnUpdate(float ts)
{
	if (!IsNetModeClient())
		return;

	if (Entity localPlayer = GetNetworkManager()->GetLocalPlayerEntity())
	{
		auto& netTransform = GetComponent<NetworkComponent>().NetTransform;

		if (netTransform.Method == NetSyncMethod::Prediction)
		{
			m_PredictionTimer += ts;
			if (m_PredictionTimer > SwitchCooldown)
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
