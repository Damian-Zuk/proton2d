#pragma once
#include "Proton/Serialization/BufferStream.h"

namespace proton {

	enum class NetMode : uint8_t
	{
		Standalone = 0,
		ListenServer = 1,
		DedicatedServer = 2,
		Client = 3,
	};

	enum class ConnectionStatus : uint8_t
	{
		Connected = 0,
		Disconnected = 1
	};

	enum class ReplicationMode : uint8_t
	{
		Normal = 0,
		Notify = 0,
	};

	enum class NetTranformSyncMethod : uint8_t
	{
		Inherit = 0,
		None = 1,
		Interpolate = 2,
		Extrapolate = 3,
		NetworkRigidbody = 4
	};

	std::string NetSyncMethodToString(NetTranformSyncMethod method);

	using Server_OnPlayerActionCallback = std::function<void(BufferStreamReader& stream)>;
	using Client_SendPlayerActionCallback = std::function<void(BufferStreamWriter& stream)>;
}
