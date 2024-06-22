#pragma once
#include "Proton/Serialization/BufferStream.h"

namespace proton {

	enum class NetMode : uint8_t
	{
		Standalone = 0,
		ListenServer = 1,
		DedicatedServer = 2, /* currently not supported */
		Client = 3,
	};

	enum class ConnectionStatus : uint8_t
	{
		Connected = 0,
		Disconnected = 1
	};

	using Server_OnPlayerActionCallback = std::function<void(BufferStreamReader& stream)>;
	using Client_SendPlayerActionCallback = std::function<void(BufferStreamWriter& stream)>;
}
