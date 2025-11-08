#pragma once
#include "Proton/Scene/Entity.h"

namespace proton {

	class EntityScript;
	class GameScript;
	class AppScript;

	using AddEntityScriptFn = std::function<EntityScript*(Entity)>;
	using AddGameScriptFn = std::function<GameScript*(Scene*)>;
	using AddAppScriptFn = std::function<AppScript*(GameInstance*)>;

	// Native Script Factory
	class ScriptFactory
	{
	public:
		static ScriptFactory& Get();

		ScriptFactory() = default;
		virtual ~ScriptFactory() = default;

		bool RegisterEntityScript(const std::string& className, const AddEntityScriptFn& addFunction);
		bool RegisterGameScript(const std::string& className, const AddGameScriptFn& addFunction);
		bool RegisterAppScript(const std::string& className, const AddAppScriptFn& addFunction);

		EntityScript* AddScriptToEntity(Entity entity, const std::string& className);
		GameScript* AddScriptToScene(Scene* scene, const std::string& className);
		AppScript* AddScriptToGameInstance(GameInstance* game, const std::string& className);

	private:
		std::unordered_map<std::string, AddEntityScriptFn> m_EntityFuncRegistry;
		std::unordered_map<std::string, AddGameScriptFn> m_GameFuncRegistry;
		std::unordered_map<std::string, AddAppScriptFn> m_AppFuncRegistry;

		friend class InspectorPanel;
	};
}

