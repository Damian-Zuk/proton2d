#pragma once
#include "Proton/Scene/Entity.h"
#include "Proton/Scripting/ScriptFactory.h"

namespace proton {

	// Supported script variable types for Serialization / Editor view.
	enum class ScriptFieldType { Float, Float2, Float3, Float4, Int, Int2, Int3, Int4, Bool };

	struct ScriptField
	{
		ScriptFieldType Type = ScriptFieldType::Float;
		void* InstanceFieldValue = nullptr;
		bool NetworkSerialize = false;
		bool ShowInEditor = true;
	};

	// Forward declaration
	class SceneManager;
	class NetworkManager;

	// Base class for entity scripts.
	// - Use ENTITY_SCRIPT_CLASS in derived classes for registration.
	// - Implement OnCreate, OnDestroy, OnUpdate for entity behavior.
	// - Use OnRegisterFields to register fields (variables) for Serialization / Editor view.
	class EntityScript : public Entity
	{
	public:
		virtual ~EntityScript() = default;

		virtual bool OnCreate() { return true; }
		virtual void OnDestroy() {}
		virtual void OnUpdate(float ts) {}
		virtual void OnPhysicsUpdate(float ts) {}

		// Register your fields (variables) here. Use PT_REGISTER_FIELD macro or RegisterField function.
		// Supported variable types are listed in ScriptFieldType enum.
		// Use PT_REPLICATE_FIELD or PT_REPLICATE_DATA here.
		virtual void OnRegisterFields() {}

		// Draw your custom ImGui Editor interface here.
		// The ImGui elements will be displayed within the script node in the Inspector Panel.
		// IMPORTANT: Ensure that your ImGui code is enclosed within #ifdef PT_EDITOR preprocessor directives.
		virtual void OnImGuiRender() {}

		// Use glm::value_ptr for FloatX and IntX field types.
		// Supported variable types are listed inside ScriptFieldType enum.
		void RegisterField(ScriptFieldType type, const std::string& name, void* field, bool showInEditor = true, bool networkSerialize = false);

		void SetPhysicsSensor(uint32_t sensorType, const std::string& childEntityTagName);
		bool CheckSensor(uint32_t sensorType) const;
		uint32_t GetSensorContactCount(uint32_t sensorType) const;

		SceneManager* GetSceneManager() const;

		virtual const std::string& GetScriptClassName() = 0;
		
		// Networking
		void ReplicateField(const std::string& name, void (*notifyFunction)(Entity*) = nullptr);
		void ReplicateData(void* data, size_t size, void (*notifyFunction)(Entity*) = nullptr);

		bool HasAuthority() const;
		bool IsRunningServer() const;
		bool IsRunningClient() const;

		// Other
		static const size_t GetFieldSize(ScriptFieldType type);

	private:
		void SetFieldValueData(const std::string& fieldName, void* value);
		
	private:
		bool m_Stopped = false;
		bool m_Initialized = false;

		std::unordered_map<std::string, ScriptField> m_ScriptFields;
		std::unordered_map<uint32_t, uint32_t*> m_PhysicsSensorMap;

		friend class Entity;
		friend class Scene;
		friend class SceneSerializer;
		friend class Client;

		friend class InspectorPanel;
	};
}

// This macro registers script class inside engine ScriptFactory.
// Script class must inherit from EntityScript class.
#define ENTITY_SCRIPT_CLASS(script_class) \
	static inline const std::string __ScriptClassName = #script_class; \
	static inline const bool __RegisteredInFactory = \
		proton::ScriptFactory::Get().RegisterScript([&](proton::Entity entity) { \
			return entity.AddScript<script_class>(); \
		}, #script_class); \
	virtual const std::string& GetScriptClassName() override { return __ScriptClassName; }

#define PT_REGISTER_FIELD(field, type, ...) \
	RegisterField(ScriptFieldType::type, #field, &field, __VA_ARGS__);

#define PT_REPLICATE_FIELD(field, ...) \
	ReplicateField(#field, __VA_ARGS__)

#define PT_REPLICATE_DATA(data, ...) \
	ReplicateData(&data, sizeof(data), __VA_ARGS__);
