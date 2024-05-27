#include "ptpch.h"
#include "Proton/Network/Common/Common.h"

namespace proton {

	std::string NetSyncMethodToString(NetTranformSyncMethod method)
	{
		switch (method)
		{
		case NetTranformSyncMethod::Inherit:
			return "Inherit";
		case NetTranformSyncMethod::None:
			return "None";
		case NetTranformSyncMethod::Interpolate:
			return "Interpolate";
		case NetTranformSyncMethod::Extrapolate:
			return "Extrapolate";
		case NetTranformSyncMethod::NetworkRigidbody:
			return "NetworkRigidbody";
		}
		return "Invalid";
	}

}

