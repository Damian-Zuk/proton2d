#include "pch.h"
#include "proton/Entity/ScriptManager.h"

namespace proton {

	Unique<ScriptManager> ScriptManager::s_Instance = nullptr;

	void ScriptManager::RegisterScript(const AddScriptToEntityFunction& addFunction, const std::string& scriptName)
	{
		s_Instance->m_RegisteredScripts[scriptName] = addFunction;
	}

	void ScriptManager::Init()
	{
		s_Instance = CreateUnique<ScriptManager>();
	}

}