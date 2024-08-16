#pragma once
#include "Proton/Core/Timer.h"

#include <deque>

namespace proton {

	struct TransformComponent; // forward declaration

	using ClientID = uint32_t;

	//enum class NetSyncMethod : uint8_t
	//{
	//	None = 0,
	//	Interpolate = 1,
	//	Extrapolate = 2,
	//	NetworkRigidbody = 3
	//};

	//struct NetSyncParams
	//{
	//	NetSyncMethod SyncMethod = NetSyncMethod::None;
	//	float ExtrapolationLimit = 0.5f;
	//	float ReconcileThreshold = 0.5f;
	//	float ReconcileCooldownTime = 1.0f;
	//	float ReconcileTime = 0.1f;
	//};

	//struct NetSyncState
	//{
	//	Timer ReplicationTimer;
	//	Timer ReconcileCooldownTimer;

	//	bool ReconcileStarted = false;
	//	bool NewUpdate = true;
	//	float PacketDelay = 0.0f;
	//};

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
			All = Position | Scale | Rotation
		};

		struct Transform
		{
			glm::vec2 Position { 0.0f, 0.0f };
			glm::vec2 Scale { 0.0f, 0.0f };
			float Rotation = 0.0f;

			bool IsZero() const;
			static Transform Get(TransformComponent* component, bool localSpace = false);
			Transform operator-(const Transform& other) const;
		};

		struct SequencedItem
		{
			uint16_t SequenceNumber;
			Transform Value;
		};

		SyncMethod Method = SyncMethod::None;
		float ReconcileThreshold = 0.5f;
		float ReconcileCooldownTime = 1.0f;
		 
		std::vector<SequencedItem> DeltaBuffer;
		uint16_t CurrentSequenceNumber = 0;
		uint16_t ServerSequenceNumber = 0;

		Transform LastTickTransform;
		Transform PredictedTransform;
		Transform LastAuthoritativeTransform;
		Transform PrevAuthoritativeTransform;
		
		ReplicationFlags Flags = ReplicationFlags::None;
		ReconcileState State = ReconcileState::None;

		bool ReplicatedThisFrame = false;
		float LastReplicationInterval = 0.0f;
		Timer ReplicationTimer;

		std::unordered_map<ClientID, SequencedItem> ClientDataMap; // server-only
	};

	using NetSyncMethod = NetTransform::SyncMethod;

	std::string NetSyncMethodToString(NetSyncMethod method);
	NetSyncMethod StringToNetSyncMethod(const std::string& syncMethod);

}
