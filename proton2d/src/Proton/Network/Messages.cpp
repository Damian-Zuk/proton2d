#include "ptpch.h"
#include "Proton/Network/Messages.h"

namespace proton {

	std::string MessageTypeToString(MessageType packetType)
	{
		switch (packetType)
		{
		case MessageType::None:
			return "MessageType::None";
		case MessageType::Handshake:
			return "MessageType::Handshake";
		case MessageType::HandshakeReply:
			return "MessageType::HandshakeReply";
		case MessageType::EntitySpawn:
			return "MessageType::EntitySpawn";
		case MessageType::EntityDespawn:
			return "MessageType::EntityDespawn";
		case MessageType::EntityReplicate:
			return "MessageType::EntityReplicate";
		case MessageType::PlayerAction:
			return "MessageType::PlayerAction";
		}
		return "MessageType::<Invalid>";
	}

}
