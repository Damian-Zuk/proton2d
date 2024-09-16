#include <Proton.h>

using namespace proton;

#include "DynamicExtrapolation.h"

void DynamicExtrapolation::OnRegisterFields()
{
	REGISTER_FIELD(Float, InterpolationThreshold);
	REGISTER_FIELD(Float, SwitchCooldownTime);
	REGISTER_FIELD(Bool, Enable);
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
	if (!Enable)
		return;

	auto& netTransform = GetComponent<NetworkComponent>().NetTransform;
	if (netTransform.Method == NetSyncMethod::Extrapolation)
		return;

	Entity parent = contact.Other->GetParent();
	if (contact.Other->HasNetworkPrediction() || (parent && parent.HasNetworkPrediction()))
	{
		if (netTransform.Method == NetSyncMethod::Interpolation)
		{
			netTransform.Method = NetSyncMethod::Extrapolation;
			m_PredictionTimer = 0.0f;
			if (AlphaVisualize) 
				GetColor().a = 1.0f;
		}
	}
}

void DynamicExtrapolation::OnFixedUpdate(float ts)
{
	if (!IsNetModeClient())
		return;

	auto& netTransform = GetComponent<NetworkComponent>().NetTransform;
	if (netTransform.Method == NetSyncMethod::Interpolation)
		return;

	m_PredictionTimer += ts;
	if (m_PredictionTimer < SwitchCooldownTime)
		return;

	auto& transform = GetTransform();
	glm::vec2 currentPosition = { transform.WorldPosition.x, transform.WorldPosition.y };
	float distance = glm::distance(currentPosition, netTransform.LastAuthoritativeTransform.Position);
			
	if (distance < InterpolationThreshold)
	{
		// Interpolate from current to last authoritative transform
		netTransform.Method = NetSyncMethod::Interpolation;
		netTransform.InterpolationTimer = 0.0f;
		if (AlphaVisualize)
			GetColor().a = 0.7f;
	}
}
