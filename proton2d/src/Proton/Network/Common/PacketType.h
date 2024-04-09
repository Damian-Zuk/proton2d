#pragma once

namespace proton {

	enum class PacketType : uint16_t
	{
		None = 0,

		// [Server -> Client]
		InitializeScene = 1,

		GameStateUpdate = 2,

		EntitySpawn = 4,
		EntityDestroy = 5,
		UpdateReplicated = 6,

		// [Client -> Server]
		PlayerAction = 5,
	};

	std::string PacketTypeToString(PacketType packetType);

}
