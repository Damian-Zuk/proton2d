#pragma once
#include "Proton/Core/Timer.h"

namespace proton {

	using ClientID = uint32_t;

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

		glm::vec2 ExtrapolatedPoint;
		float Error = 0.0f;

		bool ReconcileStarted = false;
		bool NewUpdate = true;
		float PacketDelay = 0.0f;
	};

	struct NetTransform
	{
        enum class ReplicationFlags : uint8_t
        {
            None = 0,

            PositionX = 1 << 0,
            PositionY = 1 << 1,
            Position = PositionX | PositionY,

            ScaleX = 1 << 2,
            ScaleY = 1 << 3,
            Scale = ScaleX | ScaleY,

            Rotation = 1 << 4,

            All = Position | Scale | Rotation,
        };

		struct Transform
		{
			glm::vec2 Position { 0.0f, 0.0f };
			glm::vec2 Scale { 0.0f, 0.0f };
			float Rotation = 0.0f;
		};

		ReplicationFlags RepFlags;

		NetSyncParams SyncParams;
		NetSyncState SyncState;
		Transform PreviousTransform;
		Transform CurrentTransform;
		
		std::unordered_map<ClientID, Transform> ClientToTransformMap; // server-only
	};

	std::string NetSyncMethodToString(NetSyncMethod method);
	NetSyncMethod StringToNetSyncMethod(const std::string& syncMethod);

}
