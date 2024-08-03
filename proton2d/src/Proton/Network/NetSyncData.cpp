#include "ptpch.h"
#include "Proton/Network/NetSyncData.h"

namespace proton {

	std::string NetSyncMethodToString(NetSyncMethod method)
	{
		switch (method)
		{
		case NetSyncMethod::None:
			return "None";
		case NetSyncMethod::Interpolate:
			return "Interpolate";
		case NetSyncMethod::Extrapolate:
			return "Extrapolate";
		case NetSyncMethod::NetworkRigidbody:
			return "NetworkRigidbody";
		}

		PT_CORE_ASSERT(false, "Invalid NetSyncMethod value")
		return "Invalid";
	}

	NetSyncMethod StringToNetSyncMethod(const std::string& syncMethod)
	{
		if (syncMethod == "Interpolate")
			return NetSyncMethod::Interpolate;
		else if (syncMethod == "Extrapolate")
			return NetSyncMethod::Extrapolate;
		else if (syncMethod == "NetworkRigidbody")
			return NetSyncMethod::NetworkRigidbody;
		else if (syncMethod == "None")
			return NetSyncMethod::None;

		PT_CORE_ASSERT(false, "Invalid syncMethod string value")
		return NetSyncMethod::None;
	}
}
