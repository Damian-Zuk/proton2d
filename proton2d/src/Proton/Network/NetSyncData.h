#pragma once
#include "Proton/Core/Timer.h"

namespace proton {

	enum class NetSyncMethod : uint8_t
	{
		None = 0,
		Interpolate = 1,
		Extrapolate = 2,
		NetworkRigidbody = 3
	};

	struct NetSyncParams
	{
		NetSyncMethod SyncMethod = NetSyncMethod::None;
		float ExtrapolationLimit = 0.5f;
		float ReconcileThreshold = 0.5f;
		float ReconcileTime = 0.1f;
		float ReconcileCooldownTime = 1.0f;
	};

	struct NetSyncState
	{
		Timer ExtrapolationTimer;
		Timer ReconcileTimer;
		Timer ReconcileCooldownTimer;
		Timer PacketTimer;

		glm::vec3 ExtrapolatedPoint;
		glm::vec2 EstimatedVelocity;
		float Error = 0.0f;

		bool ReconcileStarted = false;
		bool NewPacket = true;
		float PacketDelay = 0.0f;
	};

	struct NetTransform
	{
		glm::vec3 Position { 0.0f };
		glm::vec2 LinearVelocity{ 0.0f };
		float Rotation = 0.0f;
		float AngularVelocity = 0.0f;
		bool Initialized = false;
	};

	std::string NetSyncMethodToString(NetSyncMethod method);
	NetSyncMethod StringToNetSyncMethod(const std::string& syncMethod);
}

