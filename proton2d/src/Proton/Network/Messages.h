#pragma once

#include "Proton/Core/UUID.h"
#include "Proton/Network/NetTransform.h"

#define MAX_CLIENT_NAME_LENGTH 32

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
		EntitySequence,

		// [Client <-> Server]
		CustomMessage
	};

	struct MessageHandshake
	{
		MessageType MessageType = MessageType::Handshake;
		uint32_t EngineProtocolVersion;
		uint32_t GameProtocolVersion;
		char ClientName[MAX_CLIENT_NAME_LENGTH];
	};

	struct MessageHandshakeReply
	{
		MessageType MessageType = MessageType::HandshakeReply;
		uint32_t ClientID;
	};

	struct MessageEntitySpawn
	{
		MessageType MessageType = MessageType::EntitySpawn;
		uint32_t EntityCount = 0;

		struct PayloadItem
		{
			uint64_t EntityUUID = 0;
			uint64_t PrefabUUID = 0;
			uint64_t ParentUUID = 0;
			glm::vec2 Position { 0.0f };
			uint64_t PayloadSize = 0;
		};
	};

	struct MessageEntityDespawn
	{
		MessageType MessageType = MessageType::EntityDespawn;
		uint32_t EntityCount = 0;

		struct PayloadItem
		{
			UUID EntityUUID = 0;
		};
	};

	struct MessageEntityReplicate
	{
		MessageType MessageType = MessageType::EntityReplicate;
		uint32_t EntityCount = 0;

		struct PayloadItem
		{
			using Components = NetTransform::Components;
			UUID EntityUUID = 0;
			Components TransformFlags = Components::None;
			uint16_t TransformSequenceNumber = 0;
			uint32_t ScriptCount = 0;
			uint64_t PayloadSize = 0;
		};
	};

	struct MessageEntitySequence
	{
		MessageType MessageType = MessageType::EntitySequence;
		uint32_t EntityCount = 0;

		struct PayloadItem
		{
			UUID EnittyUUID = 0;
			uint16_t SequenceNumber = 0;
		};
	};

	std::string MessageTypeToString(MessageType packetType);
}
