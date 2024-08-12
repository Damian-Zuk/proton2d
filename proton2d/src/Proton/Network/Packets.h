#pragma once

#include "Proton/Core/UUID.h"
#include "Proton/Network/NetTransform.h"

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
		uint32_t ClientID;
	};

	struct NetMessageSpawn
	{
		PacketType PacketType = PacketType::EntitySpawn;
		uint32_t EntityCount;

		struct PayloadItem
		{
			std::string EntityJsonData;
			// TODO: change to:
			// uint64_t PrefabUUID;
			// uint64_t EntityUUID;
		};
	};

	struct NetMessageDespawn
	{
		PacketType PacketType = PacketType::EntityDespawn;
		uint32_t EntityCount;

		struct PayloadItem
		{
			UUID EntityUUID;
		};
	};

	struct NetMessageReplicate
	{
		PacketType PacketType = PacketType::EntityReplicate;
		uint32_t EntityCount;

		struct PayloadItem
		{
			UUID EntityUUID;
			NetTransform::ReplicationFlags TransformReplicationFlags;
			uint32_t ScriptCount;
			uint64_t PayloadSize;
		};
	};

	struct NetMassagePlayerAction
	{
		PacketType PacketType = PacketType::PlayerAction;
	};


	std::string PacketTypeToString(PacketType packetType);
}
