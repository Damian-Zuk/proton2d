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

        enum class Components : uint8_t
        {
            None = 0,
            PositionX = 1 << 0,
            PositionY = 1 << 1,
            ScaleX = 1 << 2,
            ScaleY = 1 << 3,
            Rotation = 1 << 4,
            Position = PositionX | PositionY,
            Scale = ScaleX | ScaleY,
			PositionAndRotation = Position | Rotation,
            All = Position | Scale | Rotation,
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
			uint16_t SequenceNumber;
			Transform Value;
		};

		// Parameters
		SyncMethod Method = SyncMethod::None;
		float CullDistance = 20.0f;
		float TeleportThreshold = 5.0f;
		float ReconcileThreshold = 0.1f;
		float ReconcileMaxTime = 0.1f;

		// Internal state
		std::vector<SequencedValue> DeltaBuffer;
		bool BufferHasNewDeltas = false;
		uint16_t CurrentSequenceNumber = 0;
		uint16_t ServerSequenceNumber = 0;

		Transform LastAuthoritativeTransform;
		Transform PrevAuthoritativeTransform;
		
		Transform LastTickTransform;
		Transform PredictedTransform;
		Transform ReconcileOffset;

		float ReplicationTimer = 0.0f;
		float ReconcileTimer = 0.0f;
		float InterpolationTimer = 0.0f;

		Components ReplicationFlags = Components::None;
		Components ReconcileState = Components::None;

		bool WasReplicated(Components component) const;
		bool IsReconciling(Components component) const;
		bool IsReconciling() const;

		void StartReconcile(Components component);
		void StopReconcile(Components component);
	};

	using NetSyncMethod = NetTransform::SyncMethod;
	
	std::string NetSyncMethodToString(NetSyncMethod method);
	NetSyncMethod StringToNetSyncMethod(const std::string& syncMethod);

}
