#pragma once
#include "Proton/Core/UUID.h"
#include "Proton/Serialization/BufferStream.h"
#include "Proton/Scene/Components.h"

//class EntityScript;

namespace proton {

	enum class NetMode : uint8_t
	{
		Standalone = 0,
		ListenServer = 1,
		DedicatedServer = 2,
		Client = 3,
	};

	enum class ConnectionStatus : uint8_t
	{
		Connected = 0,
		Disconnected = 1
	};

	enum class ReplicationMode : uint8_t
	{
		Standard = 0,
		Notify = 0,
	};

	//struct ReplicatedScriptField
	//{
	//	EntityScript* Instance = nullptr;
	//	std::string ScriptName; // TODO: Add UUID to script field and change std::string to UUID
	//	ReplicationMode Mode = ReplicationMode::Standard;
	//	std::function<void(EntityScript* script)> NotifyCallback = nullptr;
	//};
	//
	//struct EntityUpdateHeader
	//{
	//
	//};

	using Server_OnPlayerActionCallback = std::function<void(BufferStreamReader& stream)>;
	using Client_SendPlayerActionCallback = std::function<void(BufferStreamWriter& stream)>;
}
