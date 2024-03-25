#pragma once

namespace proton {

	enum class PacketType : uint16_t
	{
		None = 0,
		// [Server -> Client]
		InitializeScene = 1
	};

	std::string PacketTypeToString(PacketType packetType);

}
