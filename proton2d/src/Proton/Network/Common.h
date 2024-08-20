#pragma once
#include "Proton/Network/NetworkStream.h"

#define PROTON_NET_PROTOCOL_VERSION 1

namespace proton {

	using ClientID = uint32_t;

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

	enum NetConnectionEndCode
	{
		NetConnectionEndCode_ProtocolMismatch = 2001,
		NetConnectionEndCode_MaxConnections = 2002
	};

	std::string NetModeToString(NetMode netMode);
	NetMode StringToNetMode(const std::string& netModeStr);
}
