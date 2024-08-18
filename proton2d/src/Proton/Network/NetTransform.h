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

        enum class ReplicationFlags : uint8_t
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

		enum class ReconcileState : uint8_t
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
		};

		struct SequencedItem
		{
			uint16_t SequenceNumber;
			Transform Value;
		};

		// Parameters
		SyncMethod Method = SyncMethod::None;
		float CullDistance = 20.0f;
		float ReconcileThreshold = 0.5f;
		float ReconcileMaxTime = 1.0f;
		float ReconcileCooldownTime = 1.0f;

		// Internal state
		std::vector<SequencedItem> DeltaBuffer;
		uint16_t CurrentSequenceNumber = 0;
		uint16_t ServerSequenceNumber = 0;

		Transform LastTickTransform;
		Transform PredictedTransform;
		Transform LastAuthoritativeTransform;
		Transform PrevAuthoritativeTransform;

		ReplicationFlags Flags = ReplicationFlags::None;
		ReconcileState State = ReconcileState::None;

		float ReplicationInterval = 1.0f / 64.0f;
		float ReplicationTimer = 0.0f;
		float InterpolationTimer = 0.0f;

		bool IsReconciling(ReconcileState component) const;
		// State mutating function
		void StartReconcile(ReconcileState component);
		// State mutating function
		void StopReconcile(ReconcileState component);

		// Server-only data (TODO: store as separate EnTT component)
		std::unordered_map<ClientID, SequencedItem> ClientDataMap; 
	};

	using NetSyncMethod = NetTransform::SyncMethod;

	std::string NetSyncMethodToString(NetSyncMethod method);
	NetSyncMethod StringToNetSyncMethod(const std::string& syncMethod);

}
