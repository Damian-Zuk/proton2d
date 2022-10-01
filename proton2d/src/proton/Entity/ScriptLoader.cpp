#include "pch.h"
#include "proton/Entity/ScriptLoader.h"

namespace proton {

	Unique<ScriptLoader> ScriptLoader::s_Instance = nullptr;

	void ScriptLoader::LoadScript(const AddScriptToEntityFunction& addFunction, const std::string& scriptName)
	{

		s_Instance->m_AddScriptFunctions[scriptName] = addFunction;
	}

	void ScriptLoader::Init()
	{
		if (!s_Instance)
			s_Instance = CreateUnique<ScriptLoader>();
	}

}