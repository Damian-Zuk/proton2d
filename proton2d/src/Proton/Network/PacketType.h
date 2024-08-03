#pragma once

namespace proton {

	enum class PacketType : uint16_t
	{
		None = 0,

		// [Server -> Client]
		ConnectionAccepted = 1,
		GameStateUpdate = 2,

		EntitySpawn = 11,
		EntityDestroy = 12,
		UpdateReplicated = 13,

		// [Client -> Server]
		VerifyGameState = 100,
		PlayerAction = 101,
	};

	std::string PacketTypeToString(PacketType packetType);

}
