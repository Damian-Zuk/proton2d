#include "ptpch.h"
#include "Proton/Network/Packets.h"

namespace proton {

	std::string PacketTypeToString(PacketType packetType)
	{
		switch (packetType)
		{
		case PacketType::None:
			return "PacketType::None";
		case PacketType::Handshake:
			return "PacketType::Handshake";
		case PacketType::HandshakeReply:
			return "PacketType::HandshakeReply";
		case PacketType::EntitySpawn:
			return "PacketType::EntitySpawn";
		case PacketType::EntityDespawn:
			return "PacketType::EntityDespawn";
		case PacketType::EntityReplicate:
			return "PacketType::EntityReplicate";
		case PacketType::PlayerAction:
			return "PacketType::PlayerAction";
		}
		return "PacketType::<Invalid>";
	}

}
