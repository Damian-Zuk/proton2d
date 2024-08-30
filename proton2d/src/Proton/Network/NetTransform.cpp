#include "ptpch.h"
#include "Proton/Network/NetTransform.h"
#include "Proton/Scene/Components.h"

#include <glm/gtx/norm.hpp>

namespace proton {

	using Transform = NetTransform::Transform;

	std::string NetSyncMethodToString(NetSyncMethod method)
	{
		switch (method)
		{
		case NetSyncMethod::Interpolation:
			return "Interpolation";
		case NetSyncMethod::Extrapolation:
			return "Extrapolation";
		case NetSyncMethod::Prediction:
			return "Prediction";
		}
		return "None";
	}

	NetSyncMethod StringToNetSyncMethod(const std::string& syncMethod)
	{
		if (syncMethod == "Interpolation")
			return NetSyncMethod::Interpolation;
		else if (syncMethod == "Extrapolation")
			return NetSyncMethod::Extrapolation;
		else if (syncMethod == "Prediction")
			return NetSyncMethod::Prediction;
		return NetSyncMethod::None;
	}

	bool Transform::IsZero() const
	{
		constexpr float epsilon = 1e-4f;
		return glm::compMax(glm::abs(Position)) < epsilon
			&& glm::compMax(glm::abs(Scale)) < epsilon
			&& glm::abs(Rotation) < epsilon;
	}

	bool NetTransform::Transform::IsNotZero() const
	{
		return !IsZero();
	}

	Transform Transform::Get(TransformComponent* component, bool localSpace)
	{
		Transform transform;
		transform.Position = {
			localSpace ? component->LocalPosition.x : component->WorldPosition.x,
			localSpace ? component->LocalPosition.y : component->WorldPosition.y
		};
		transform.Scale = {
			component->Scale.x, component->Scale.y
		};
		transform.Rotation = component->Rotation;
		return transform;
	}

	Transform Transform::operator-(const Transform& other) const
	{
		return Transform {
			this->Position - other.Position,
			this->Scale - other.Scale,
			this->Rotation - other.Rotation,
		};
	}

	Transform NetTransform::Transform::operator+(const Transform& other) const
	{
		return Transform {
			this->Position + other.Position,
			this->Scale + other.Scale,
			this->Rotation + other.Rotation,
		};
	}

	bool NetTransform::WasReplicated(Components component) const
	{
		return EnumHasAnyFlags(ReplicationFlags, component);
	}

	bool NetTransform::IsReconciling(Components component) const
	{
		return EnumHasAllFlags(ReconcileState, component);
	}

	bool NetTransform::IsReconciling() const
	{
		return EnumHasAnyFlags(ReconcileState, Components::All);
	}

	void NetTransform::StartReconcile(Components component)
	{
		ReconcileState = EnumAddFlags(ReconcileState, component);
	}

	void NetTransform::StopReconcile(Components component)
	{
		ReconcileState = EnumRemoveFlags(ReconcileState, component);
		ReconcileTimer = 0.0f;
	}

}
