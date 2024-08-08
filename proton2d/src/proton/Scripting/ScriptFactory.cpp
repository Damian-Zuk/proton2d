#include "ptpch.h"
#include "Proton/Scripting/ScriptFactory.h"
#include "Proton/Scripting/GameModeBase.h"

namespace proton {

	ScriptFactory::ScriptFactory()
	{
		m_GameModeRegistry["GameModeBase"] = [&](Scene* scene) {
			return scene->SetGameMode<GameModeBase>();
		};
	}

	ScriptFactory& ScriptFactory::Get()
	{
		static ScriptFactory instance;
		return instance;
	}

	EntityScript* ScriptFactory::AddScriptToEntity(Entity entity, const std::string& className)
	{
		if (m_ScriptRegistry.find(className) == m_ScriptRegistry.end())
		{
			PT_CORE_ERROR("Script '{}' not found!", className);
			return nullptr;
		}
		AddScriptFunction& addScriptFunction = m_ScriptRegistry.at(className);
		EntityScript* script = addScriptFunction(entity);
		return script;
	}

	bool ScriptFactory::RegisterScript(const AddScriptFunction& addFunction, const std::string& className)
	{
		if (m_ScriptRegistry.find(className) != m_ScriptRegistry.end())
		{
			PT_CORE_ERROR("Script '{}' already exists", className);
			return false;
		}
		m_ScriptRegistry[className] = addFunction;
		return true;
	}

	GameModeBase* ScriptFactory::InstantiateGameMode(Scene* scene, const std::string& className)
	{
		if (m_GameModeRegistry.find(className) == m_GameModeRegistry.end())
		{
			PT_CORE_ERROR("GameMode '{}' not found!", className);
			return nullptr;
		}
		AddGameModeFunction& instantiateFunction = m_GameModeRegistry.at(className);
		GameModeBase* gameMode = instantiateFunction(scene);
		return gameMode;
	}

	bool ScriptFactory::RegisterGameMode(const AddGameModeFunction& function, const std::string& className)
	{
		if (m_GameModeRegistry.find(className) != m_GameModeRegistry.end())
		{
			PT_CORE_ERROR("GameMode '{}' already exists", className);
			return false;
		}
		m_GameModeRegistry[className] = function;
		return true;
	}

}
