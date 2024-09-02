#include "ptpch.h"
#include "Proton/Network/Common.h"

namespace proton {

	std::string NetModeToString(NetMode netMode)
	{
		switch (netMode)
		{
		case NetMode::Standalone:
			return "Standalone";
		case NetMode::ListenServer:
			return "ListenServer";
		case NetMode::DedicatedServer:
			return "DedicatedServer";
		case NetMode::Client:
			return "Client";
		}
		return "Standalone";
	}

	NetMode StringToNetMode(const std::string& netModeStr)
	{
		if (netModeStr == "ListenServer")
			return NetMode::ListenServer;
		if (netModeStr == "DedicatedServer")
			return NetMode::DedicatedServer;
		if (netModeStr == "Client")
			return NetMode::Client;
		return NetMode::Standalone;
	}

}
