#include "ptpch.h"
#include "Proton/Network/Common/PacketType.h"

namespace proton {

	std::string PacketTypeToString(PacketType packetType)
	{
		switch (packetType)
		{
		case PacketType::None:
			return "PacketType::None";
		case PacketType::ConnectionAccepted:
			return "PacketType::ConnectionAccepted";
		case PacketType::EntitySpawn:
			return "PacketType::EntitySpawn";
		case PacketType::EntityDestroy:
			return "PacketType::EntityDestroy";
		case PacketType::UpdateReplicated:
			return "PacketType::EntityDestroy";
		case PacketType::PlayerAction:
			return "PacketType::PlayerAction";
		}
		return "PacketType::<Invalid>";
	}

}
