#pragma once

#include "Proton/Core/UUID.h"

namespace proton {

	enum class PacketType : uint16_t
	{
		None = 0, // Invalid packet

		// [Client -> Server]
		Handshake = 1,

		// [Server -> Client]
		HandshakeReply = 2,

		// [Server -> Client]
		EntitySpawn,
		EntityDespawn,
		EntityReplicate,

		PlayerAction, // Delete me

		// [Client <-> Server]
		Custom
	};

	struct NetMessageHandshake
	{
		PacketType PacketType = PacketType::Handshake;
		uint32_t EngineProtocolVersion;
		uint32_t GameProtocolVersion;
	};

	struct NetMessageHandshakeReply
	{
		PacketType PacketType = PacketType::HandshakeReply;
		uint16_t ResultCode;
		uint32_t ClientID;
	};

	struct NetMessageSpawn
	{
		PacketType PacketType = PacketType::EntitySpawn;
		uint32_t EntityCount;
	};

	struct NetMessageDespawn
	{
		PacketType PacketType = PacketType::EntityDespawn;
		uint32_t EntityCount;
	};

	struct NetMessageReplicate
	{
		PacketType PacketType = PacketType::EntityReplicate;
		uint32_t EntityCount;
	};

	struct NetMessageReplicate_Entry
	{
		UUID EntityUUID;
		std::bitset<128> ComponentBitset;
		uint64_t PayloadSize;
	};

	struct NetMassagePlayerAction
	{
		PacketType PacketType = PacketType::PlayerAction;
	};


	std::string PacketTypeToString(PacketType packetType);
}
