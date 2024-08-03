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

	using StreamReaderDelegate = std::function<void(BufferStreamReader& stream)>;
	using StreamWriterDelegate = std::function<void(BufferStreamWriter& stream)>;

	std::string NetModeToString(NetMode netMode);

	NetMode StringToNetMode(const std::string& netModeStr);
}
