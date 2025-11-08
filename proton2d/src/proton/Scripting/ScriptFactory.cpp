#include "ptpch.h"
#include "Proton/Scripting/ScriptFactory.h"
#include "Proton/Scripting/AppScript.h"
#include "Proton/Scripting/GameScript.h"
#include "Proton/Scripting/EntityScript.h"
#include "Proton/Scripting/Default/AppScriptDefault.h"
#include "Proton/Scripting/Default/GameScriptDefault.h"

namespace proton {

	ScriptFactory& ScriptFactory::Get()
	{
		static ScriptFactory instance;
		return instance;
	}

	EntityScript* ScriptFactory::AddScriptToEntity(Entity entity, const std::string& className)
	{
		if (m_EntityFuncRegistry.find(className) == m_EntityFuncRegistry.end())
		{
			PT_CORE_ERROR("Script '{}' not found!", className);
			return nullptr;
		}
		AddEntityScriptFn& addScriptFn = m_EntityFuncRegistry.at(className);
		return addScriptFn(entity);
	}

	GameScript* ScriptFactory::AddScriptToScene(Scene* scene, const std::string& className)
	{
		if (m_GameFuncRegistry.find(className) == m_GameFuncRegistry.end())
		{
			if (className != "GameScriptDefault")
				PT_CORE_ERROR("Script '{}' not found! Using GameScriptDefault.", className);
			return scene->SetGameScript<GameScriptDefault>();
		}
		AddGameScriptFn& addScriptFn = m_GameFuncRegistry.at(className);
		return addScriptFn(scene);
	}

	AppScript* ScriptFactory::AddScriptToGameInstance(GameInstance* gameInstance, const std::string& className)
	{
		if (m_AppFuncRegistry.find(className) == m_AppFuncRegistry.end())
		{
			if (className != "AppScriptDefault")
				PT_CORE_ERROR("Script '{}' not found! Using AppScriptDefault.", className);
			return gameInstance->SetAppScript<AppScriptDefault>();
		}
		AddAppScriptFn& addScriptFn = m_AppFuncRegistry.at(className);
		return addScriptFn(gameInstance);
	}

	bool ScriptFactory::RegisterEntityScript(const std::string& className, const AddEntityScriptFn& addFunction)
	{
		if (m_EntityFuncRegistry.find(className) != m_EntityFuncRegistry.end())
		{
			PT_CORE_ERROR("Script '{}' already exists", className);
			return false;
		}
		m_EntityFuncRegistry[className] = addFunction;
		return true;
	}

	bool ScriptFactory::RegisterGameScript(const std::string& className, const AddGameScriptFn& addFunction)
	{
		if (m_GameFuncRegistry.find(className) != m_GameFuncRegistry.end())
		{
			PT_CORE_ERROR("Script '{}' already exists", className);
			return false;
		}
		m_GameFuncRegistry[className] = addFunction;
		return true;
	}

	bool ScriptFactory::RegisterAppScript(const std::string& className, const AddAppScriptFn& addFunction)
	{
		if (m_AppFuncRegistry.find(className) != m_AppFuncRegistry.end())
		{
			PT_CORE_ERROR("Script '{}' already exists", className);
			return false;
		}
		m_AppFuncRegistry[className] = addFunction;
		return true;
	}

}
