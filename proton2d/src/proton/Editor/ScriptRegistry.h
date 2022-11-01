#pragma once

#ifndef PROTON_DISTRIBUTION
	#define REGISTER_SCRIPT(__script_class_type)                           \
		ScriptRegistry::RegisterScript( [&](Entity entity) {               \
			entity.AddScript<__script_class_type>(#__script_class_type);   \
		}, #__script_class_type);
#else
	#define EDITOR_REGISTER_SCRIPT(__script_class_type) 
#endif 

namespace proton {

	class Entity;

	class ScriptRegistry
	{
	public:
		using AddScriptToEntityFunction = std::function<void(Entity entity)>;
		
		static void Init();

		static void RegisterScript(const AddScriptToEntityFunction& addFunction, const std::string& scriptName);

	private:
		static Unique<ScriptRegistry> s_Instance;

		std::unordered_map<std::string, AddScriptToEntityFunction> m_AddScriptFunctions;

		friend class Inspector;
	};
}