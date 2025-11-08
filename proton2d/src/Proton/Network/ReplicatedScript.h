#pragma once

namespace proton {

	class Script;

	using ClientID = uint32_t;
	using NotifyFunction = std::function<void()>;

	struct ReplicatedField
	{
		ScriptField Field;
		NotifyFunction NotifyFunction;

		// Server-only data. TODO: Store somewhere else
		std::unordered_map<ClientID, uint32_t> ClientChecksumMap;
	};

	using ReplicatedFields = std::vector<ReplicatedField>;

	struct ReplicatedScript
	{
		ReplicatedFields ReplicatedFields;
		Script* Script = nullptr;
	};

	using ReplicatedScripts = std::vector<ReplicatedScript>;

}
