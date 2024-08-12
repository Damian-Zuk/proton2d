#include "ptpch.h"
#include "Proton/Network/NetTransform.h"

namespace proton {

	std::string NetSyncMethodToString(NetSyncMethod method)
	{
		switch (method)
		{
		case NetSyncMethod::Interpolate:
			return "Interpolate";
		case NetSyncMethod::Extrapolate:
			return "Extrapolate";
		case NetSyncMethod::NetworkRigidbody:
			return "NetworkRigidbody";
		}
		
		return "None";
	}

	NetSyncMethod StringToNetSyncMethod(const std::string& syncMethod)
	{
		if (syncMethod == "Interpolate")
			return NetSyncMethod::Interpolate;
		else if (syncMethod == "Extrapolate")
			return NetSyncMethod::Extrapolate;
		else if (syncMethod == "NetworkRigidbody")
			return NetSyncMethod::NetworkRigidbody;
		
		return NetSyncMethod::None;
	}

}
