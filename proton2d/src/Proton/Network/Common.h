#pragma once
#include "Proton/Network/NetworkStream.h"

#define PROTON_NET_PROTOCOL_VERSION 1

namespace proton {

	using ClientID = uint32_t; // HSteamNetConnection

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

	enum ConnectionEndCode : uint32_t
	{
		ConnectionEndCode_Kicked = 0,
		ConnectionEndCode_Timeout = 1,
		ConnectionEndCode_LostConnection = 2,
		ConnectionEndCode_FailedToHandshake = 2001,
		ConnectionEndCode_ProtocolMismatch = 2002,
		ConnectionEndCode_MaxConnections = 2003,
		ConnectionEndCode_ServerShutdown = 2004,
		ConnectionEndCode_Disconnected = 2005,
	};

	struct ClientInfo
	{
		ClientID ID;
		ConnectionStatus Status;
		std::string ClientName;
		std::string ConnectionDesc;
	};

	std::string NetModeToString(NetMode netMode);
	NetMode StringToNetMode(const std::string& netModeStr);
}
