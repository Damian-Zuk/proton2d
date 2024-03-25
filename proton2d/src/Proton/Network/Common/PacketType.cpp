#include "ptpch.h"
#include "Proton/Network/Common/PacketType.h"

namespace proton {

	std::string PacketTypeToString(PacketType packetType)
	{
		switch (packetType)
		{
		case PacketType::None:
			return "PacketType::None";
		case PacketType::InitializeScene:
			return "PacketType::InitializeScene";
		}
		return "PacketType::<Invalid>";
	}

}
