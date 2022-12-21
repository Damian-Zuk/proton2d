#include "pch.h"
#include "proton/Scene/ScriptFactory.h"

namespace proton {

	void ScriptFactory::RegisterScript(const AddScriptToEntityFunction& addFunction, const std::string& scriptName)
	{
		Get().m_AddScriptFunctions[scriptName] = addFunction;
	}

	ScriptFactory& ScriptFactory::Get()
	{
		static ScriptFactory instance;
		return instance;
	}

	const std::unordered_map<std::string, AddScriptToEntityFunction>& ScriptFactory::GetScripts()
	{
		return Get().m_AddScriptFunctions;
	}

}