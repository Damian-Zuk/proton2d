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
		Transform delta;
		delta.Position = this->Position - other.Position;
		delta.Scale = this->Scale - other.Scale;
		delta.Rotation = this->Rotation - other.Rotation;
		return delta;
	}

	bool NetTransform::IsReconciling(ReconcileState component) const
	{
		return EnumHasAllFlags(State, component);
	}

	void NetTransform::StartReconcile(ReconcileState component)
	{
		State = EnumAddFlags(State, component);
	}

	void NetTransform::StopReconcile(ReconcileState component)
	{
		State = EnumRemoveFlags(State, component);
	}

}
