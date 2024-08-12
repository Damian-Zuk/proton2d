#pragma once
#include "Proton/Serialization/BufferStream.h"

#define PT_NET_PROTOCOL_VERSION 1

namespace proton {

	enum class NetMode : uint8_t
	{
		Standalone = 0,
		ListenServer,
		DedicatedServer,
		Client,
	};

	enum class ConnectionStatus : uint8_t
	{
		Disconnected = 0,
		Connecting,
		Connected,
		FailedToConnect
	};

	using StreamReaderDelegate = std::function<void(BufferStreamReader& stream)>;
	using StreamWriterDelegate = std::function<void(BufferStreamWriter& stream)>;

	std::string NetModeToString(NetMode netMode);
	NetMode StringToNetMode(const std::string& netModeStr);
}
