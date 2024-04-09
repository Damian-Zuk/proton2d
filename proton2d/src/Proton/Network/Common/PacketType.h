#pragma once

namespace proton {

	enum class PacketType : uint16_t
	{
		None = 0,

		// [Server -> Client]
		ConnectionAccepted = 1,
		InitializeScene = 2,

		GameStateUpdate = 10,

		EntitySpawn = 11,
		EntityDestroy = 12,
		UpdateReplicated = 13,

		// [Client -> Server]
		PlayerAction = 100,
	};

	std::string PacketTypeToString(PacketType packetType);

}
