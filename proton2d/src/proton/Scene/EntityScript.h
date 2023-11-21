#pragma once
#include "proton/Scene/Entity.h"
#include "proton/Scene/ScriptFactory.h"

// ENTITY_SCRIPT_CLASS: Registers a script class in ScriptFactory for an Entity.
// script_class: Class name to be registered.
#define ENTITY_SCRIPT_CLASS(script_class) \
static inline const char __ScriptClassName[] = #script_class; \
static inline const bool __RegisteredInFactory = \
	proton::ScriptFactory::Get().RegisterScript([&](proton::Entity entity) { \
		return entity.AddScript<script_class>(); \
	}, #script_class);

namespace proton {

	enum class ScriptFieldType { Float = 0, Float2, Float3, Float4, Int, Int2, Int3, Int4, Bool };

	struct ScriptField
	{
		ScriptFieldType Type = ScriptFieldType::Float;
		void* InstanceFieldValue = nullptr;
	};

	// EntityScript: Base class for entity scripts.
	// - Use ENTITY_SCRIPT_CLASS in derived classes for registration.
	// - Implement OnCreate, OnDestroy, OnUpdate for entity behavior.
	// - Use OnRegisterFields to register fields for serialization/editor.
	class EntityScript
	{
	public:
		virtual ~EntityScript() = default;

		virtual void OnCreate() {}
		virtual void OnDestroy() {}
		virtual void OnUpdate(float ts) {}

		// Register script fields using RegisterField.
		// Fields are shown
		virtual void OnRegisterFields() {}

		// Registers a field with the script.
		// Use glm::value_ptr for FloatX and IntX field types.
		void RegisterField(ScriptFieldType type, const std::string& name, void* field) {
			m_ScriptFields[name] = { type, field };
		}

	protected:
		Entity m_Entity;
		
	private:
		bool m_Initialized = false;
		std::unordered_map<std::string, ScriptField> m_ScriptFields;

		friend class Scene;
		friend class InspectorPanel;
		friend class SceneSerializer;
	};
}
