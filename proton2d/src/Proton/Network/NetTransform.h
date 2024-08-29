#pragma once
#include "Proton/Core/Timer.h"

#include <deque>

namespace proton {

	struct TransformComponent; // forward declaration

	using ClientID = uint32_t;

	struct NetTransform
	{
		enum class SyncMethod : uint8_t
		{
			None = 0,
			Interpolation,
			Extrapolation,
			Prediction,
		};

        enum class ReplicateComponents : uint8_t
        {
            None = 0,
            PositionX = 1 << 0,
            PositionY = 1 << 1,
            ScaleX = 1 << 2,
            ScaleY = 1 << 3,
            Rotation = 1 << 4,
            Position = PositionX | PositionY,
            Scale = ScaleX | ScaleY,
            All = Position | Scale | Rotation,
        };

		enum class ReconcileComponents : uint8_t
		{
			None = 0,
			Position = 1 << 0,
			Scale = 1 << 1,
			Rotation = 1 << 2,
			PositionAndRotation = Position | Rotation,
			All = Position | Scale | Rotation
		};

		struct Transform
		{
			glm::vec2 Position { 0.0f, 0.0f };
			glm::vec2 Scale { 0.0f, 0.0f };
			float Rotation = 0.0f;

			bool IsZero() const;
			bool IsNotZero() const;
			static Transform Get(TransformComponent* component, bool localSpace = false);
			Transform operator-(const Transform& other) const;
			Transform operator+(const Transform& other) const;
		};

		struct SequencedValue
		{
			uint32_t SequenceNumber;
			Transform Value;
		};

		// Parameters
		SyncMethod Method = SyncMethod::None;
		float CullDistance = 20.0f;
		float ReconcileThreshold = 0.5f; // position only
		float ReconcileTime = 1.0f; // position only
		float ReconcileCooldownTime = 1.0f; // position only
		float TeleportThreshold = 5.0f;

		// Internal state
		std::vector<SequencedValue> DeltaBuffer;
		uint32_t CurrentSequenceNumber = 0;
		uint32_t ServerSequenceNumber = 0;
		bool HasNewDeltas = false;

		Transform LastAuthoritativeTransform;
		Transform PrevAuthoritativeTransform;
		
		Transform LastTickTransform;
		Transform PredictedTransform;
		Transform ReconcileOffset;

		float ReplicationTimer = 0.0f; // time elapsed from last replication
		float ReconcileTimer = 0.0f; // negative value means on cooldown
		float InterpolationTimer = 0.0f;

		ReplicateComponents ReplicationFlags = ReplicateComponents::None;
		ReconcileComponents ReconciliationState = ReconcileComponents::None;

		bool IsReconciling(ReconcileComponents component) const;
		// State mutating function
		void StartReconcile(ReconcileComponents component);
		// State mutating function
		void StopReconcile(ReconcileComponents component);

		// Server-only data (TODO: store as separate EnTT component)
		std::unordered_map<ClientID, SequencedValue> ServerDataMap;
	};

	using NetSyncMethod = NetTransform::SyncMethod;

	std::string NetSyncMethodToString(NetSyncMethod method);
	NetSyncMethod StringToNetSyncMethod(const std::string& syncMethod);

}
