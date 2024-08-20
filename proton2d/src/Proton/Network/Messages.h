#pragma once

#include "Proton/Core/UUID.h"
#include "Proton/Network/NetTransform.h"

namespace proton {

	enum class MessageType : uint16_t
	{
		None = 0, // Invalid message type

		// [Client -> Server]
		Handshake,

		// [Server -> Client]
		HandshakeReply,

		// [Server -> Client]
		EntitySpawn,
		EntityDespawn,
		EntityReplicate,

		// [Client -> Server]
		EntityTransformSequence,

		PlayerAction, // Delete me

		// [Client <-> Server]
		CustomMessage
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
		uint32_t EntityCount = 0;

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
		uint32_t EntityCount = 0;

		struct PayloadItem
		{
			UUID EntityUUID;
		};
	};

	struct NetMessageReplicate
	{
		MessageType MessageType = MessageType::EntityReplicate;
		uint32_t EntityCount = 0;

		struct PayloadItem
		{
			using ReplicateComponents = NetTransform::ReplicateComponents;

			UUID EntityUUID;
			ReplicateComponents TransformFlags = ReplicateComponents::None;
			uint16_t TransformSequenceNumber = 0;
			uint32_t ScriptCount;
			uint64_t PayloadSize;
		};
	};

	struct NetMessageTransformSequence
	{
		MessageType MessageType = MessageType::EntityTransformSequence;
		uint32_t EntityCount = 0;

		struct PayloadItem
		{
			UUID EnittyUUID;
			uint16_t SequenceNumber;
		};
	};

	struct NetMassagePlayerAction
	{
		MessageType MessageType = MessageType::PlayerAction;
	};

	std::string MessageTypeToString(MessageType packetType);
}
