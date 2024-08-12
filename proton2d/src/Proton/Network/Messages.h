#pragma once

#include "Proton/Core/UUID.h"
#include "Proton/Network/NetTransform.h"

namespace proton {

	enum class MessageType : uint16_t
	{
		None = 0, // Invalid message

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
		MessageType MessageType = MessageType::Handshake;
		uint32_t EngineProtocolVersion;
		uint32_t GameProtocolVersion;
	};

	struct NetMessageHandshakeReply
	{
		MessageType MessageType = MessageType::HandshakeReply;
		uint32_t ClientID;
	};

	struct NetMessageSpawn
	{
		MessageType MessageType = MessageType::EntitySpawn;
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
		MessageType MessageType = MessageType::EntityDespawn;
		uint32_t EntityCount;

		struct PayloadItem
		{
			UUID EntityUUID;
		};
	};

	struct NetMessageReplicate
	{
		MessageType MessageType = MessageType::EntityReplicate;
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
		MessageType MessageType = MessageType::PlayerAction;
	};


	std::string MessageTypeToString(MessageType packetType);
}
