#pragma once

namespace proton {

	class Entity;
	class EntityScript;
	using AddScriptToEntityFunction = std::function<EntityScript*(Entity entity)>;

	class ScriptFactory
	{
	public:
		static ScriptFactory& Get();

		const std::unordered_map<std::string, AddScriptToEntityFunction>& GetScripts();
		
		void RegisterScript(const AddScriptToEntityFunction& addFunction, const std::string& scriptName);

	private:
		std::unordered_map<std::string, AddScriptToEntityFunction> m_AddScriptFunctions;
	};
}