#pragma once

#define LOAD_SCRIPT(__script_class_type)                               \
	ScriptLoader::LoadScript( [&](Entity entity) {                     \
		entity.AddScript<__script_class_type>(#__script_class_type);   \
    }, #__script_class_type);

namespace proton {

	// Forward declaration
	class Entity;

	using AddScriptToEntityFunction = std::function<void(Entity entity)>;

	class ScriptLoader
	{
	public:
		static void Init();

		static void LoadScript(const AddScriptToEntityFunction& addFunction, const std::string& scriptName);

	private:
		static Unique<ScriptLoader> s_Instance;

		std::unordered_map<std::string, AddScriptToEntityFunction> m_AddScriptFunctions;

		friend class Inspector;
	};
}