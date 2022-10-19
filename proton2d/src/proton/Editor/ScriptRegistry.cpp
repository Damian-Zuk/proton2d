#include "pch.h"
#include "proton/Editor/ScriptRegistry.h"

namespace proton {

	Unique<ScriptRegistry> ScriptRegistry::s_Instance = nullptr;

	void ScriptRegistry::RegisterScript(const AddScriptToEntityFunction& addFunction, const std::string& scriptName)
	{
		s_Instance->m_AddScriptFunctions[scriptName] = addFunction;
	}

	void ScriptRegistry::Init()
	{
		if (!s_Instance)
			s_Instance = CreateUnique<ScriptRegistry>();
	}

}