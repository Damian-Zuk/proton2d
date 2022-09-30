#pragma once

#define REGISTER_SCRIPT(__script_class_type)                                  \
	ScriptManager::RegisterScript([&](Entity entity) {                        \
		entity.AddScriptComponent<__script_class_type>(#__script_class_type); \
	}, #__script_class_type);

namespace proton {

	// Forward declaration
	class Entity;

	using AddScriptToEntityFunction = std::function<void(Entity entity)>;


	class ScriptManager
	{
	public:
		static void RegisterScript(const AddScriptToEntityFunction& addFunction, const std::string& scriptName);

		static void Init();

	private:
		static Unique<ScriptManager> s_Instance;

		std::unordered_map<std::string, AddScriptToEntityFunction> m_RegisteredScripts;

		friend class Inspector;
	};
}