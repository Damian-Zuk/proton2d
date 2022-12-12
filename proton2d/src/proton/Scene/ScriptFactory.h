#pragma once

#define REGISTER_SCRIPT(__script_class_type)                                \
	ScriptFactory::RegisterScript( [&](Entity entity) {                     \
		return entity.AddScript<__script_class_type>(#__script_class_type); \
	}, #__script_class_type);


namespace proton {

	class Entity;
	class EntityScript;
	using AddScriptToEntityFunction = std::function<EntityScript*(Entity entity)>;

	class ScriptFactory
	{
	public:
		
		static void Init();

		static const std::unordered_map<std::string, AddScriptToEntityFunction>& GetScripts();
		
		static void RegisterScript(const AddScriptToEntityFunction& addFunction, const std::string& scriptName);

	private:
		static Unique<ScriptFactory> s_Instance;
		std::unordered_map<std::string, AddScriptToEntityFunction> m_AddScriptFunctions;
	};
}