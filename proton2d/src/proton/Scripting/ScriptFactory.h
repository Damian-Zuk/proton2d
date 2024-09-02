#pragma once
#include "Proton/Scene/Entity.h"

namespace proton {

	class EntityScript;
	using AddScriptFunction = std::function<EntityScript*(Entity entity)>;
	using AddGameModeFunction = std::function<GameModeBase* (Scene* scene)>;

	class ScriptFactory
	{
	public:
		ScriptFactory();
		
		static ScriptFactory& Get(); // Get singleton instance

		EntityScript* AddScriptToEntity(Entity entity, const std::string& className);
		bool RegisterScript(const AddScriptFunction& addFunction, const std::string& className);

		GameModeBase* InstantiateGameMode(Scene* scene, const std::string& className);
		bool RegisterGameMode(const AddGameModeFunction& function, const std::string& className);

	private:
		std::unordered_map<std::string, AddScriptFunction> m_ScriptRegistry;
		std::unordered_map<std::string, AddGameModeFunction> m_GameModeRegistry;

		friend class InspectorPanel;
	};
}
