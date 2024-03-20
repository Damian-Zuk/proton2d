#pragma once

enum class NetMode : uint8_t
{
	Standalone = 0,
	ListenServer = 1,
	DedicatedServer = 2,
	Client = 3,
};
