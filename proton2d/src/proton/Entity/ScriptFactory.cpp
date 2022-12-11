#include "pch.h"
#include "proton/Entity/ScriptFactory.h"

namespace proton {

	Unique<ScriptFactory> ScriptFactory::s_Instance = nullptr;

	void ScriptFactory::RegisterScript(const AddScriptToEntityFunction& addFunction, const std::string& scriptName)
	{
		s_Instance->m_AddScriptFunctions[scriptName] = addFunction;
	}

	void ScriptFactory::Init()
	{
		if (!s_Instance)
			s_Instance = CreateUnique<ScriptFactory>();
	}

	const std::unordered_map<std::string, AddScriptToEntityFunction>& ScriptFactory::GetScripts()
	{
		return s_Instance->m_AddScriptFunctions;
	}

}