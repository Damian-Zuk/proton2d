#pragma once
#include "Proton/Scene/Entity.h"
#include "Proton/Scripting/ScriptFactory.h"
#include "Proton/Core/KeyCodes.h"

// This macro registers script class inside ScriptFactory.
// Script class must inherit from EntityScript class.
#define ENTITY_SCRIPT_CLASS(script_class) \
	static inline const std::string __ScriptClassName = #script_class; \
	static inline const bool __RegisteredInFactory = \
		proton::ScriptFactory::Get().RegisterScript([&](proton::Entity entity) { \
			return entity.AddScript<script_class>(); \
		}, #script_class); \
	virtual const std::string& GetScriptClassName() override { return __ScriptClassName; }

#define REGISTER_FIELD(type, field) \
	RegisterField(ScriptFieldType::type, #field, &field, true);

#define REGISTER_FIELD_NO_EDIT(type, field) \
	RegisterField(ScriptFieldType::type, #field, &field, false);

#define REPLICATED_FIELD(field, ...) \
	SetReplicatedField(#field, __VA_ARGS__)

#define REPLICATED_DATA(data, ...) \
	SetReplicatedData(&data, sizeof(data), __VA_ARGS__);

namespace proton {

	// Forward declaration
	class SceneManager;
	class NetworkManager;

	// Supported script variable types for Serialization / Editor view.
	enum class ScriptFieldType { Float, Float2, Float3, Float4, Int, Int2, Int3, Int4, Bool };

	struct ScriptField
	{
		ScriptFieldType Type = ScriptFieldType::Float;
		void* InstanceFieldValue = nullptr;
		bool ShowInEditor = true;
	};

	// Base class for entity scripts.
	// - Use ENTITY_SCRIPT_CLASS in derived classes for script class registration.
	// - Implement OnCreate, OnDestroy, OnUpdate for entity behavior.
	// - Use OnRegisterFields method to register fields (variables) for serialization / editor view.
	class EntityScript : public Entity
	{
	public:
		virtual ~EntityScript() = default;

		virtual bool OnCreate() { return true; }
		virtual void OnDestroy() {}
		virtual void OnUpdate(float ts) {}
		virtual void OnPhysicsUpdate(float ts) {}

		// Register your fields (variables) in this method.
		// Supported variable types are contained in ScriptFieldType enum.
		// Call REGISTER_FIELD, REPLICATED_FIELD, REPLICATED_DATA macros here.
		virtual void OnRegisterFields() {}

		// Draw your custom ImGui Editor interface here.
		// The ImGui elements will be displayed within the script node in the Inspector Panel.
		// IMPORTANT: Ensure that your ImGui code is enclosed within #ifdef PT_EDITOR preprocessor directives.
		virtual void OnImGuiRender() {}

		// Use glm::value_ptr for FloatX and IntX field types.
		// Supported variable types are listed inside ScriptFieldType enum.
		void RegisterField(ScriptFieldType type, const std::string& name, void* field, bool showInEditor = true);

		// Scene
		SceneManager* GetSceneManager() const;
		
		// Physics
		void SetPhysicsSensor(uint32_t sensorType, const std::string& childEntityTagName);
		bool CheckSensor(uint32_t sensorType) const;
		uint32_t GetSensorContactCount(uint32_t sensorType) const;

		// Input
		bool IsKeyPressed(KeyCode key) const;
		bool IsMouseButtonPressed(MouseCode button) const;
		
		// Networking
		NetworkManager* GetNetworkManager() const;

		void SetReplicatedField(const std::string& name, const std::function<void()>& notifyFunction = nullptr);
		void SetReplicatedData(void* data, size_t size, const std::function<void()>& notifyFunction = nullptr);

		bool HasAuthority() const;
		bool IsRunningServer() const;
		bool IsRunningClient() const;

		bool HasNetworkPrediction() const;

		// Other
		static const size_t GetFieldSize(ScriptFieldType type);
		virtual const std::string& GetScriptClassName() = 0; // Defined by ENTITY_SCRIPT_CLASS macro

	private:
		void SetFieldValueData(const std::string& fieldName, void* value);
		
	private:
		bool m_Initialized = false;
		bool m_FailedToInitialize = false;

		std::unordered_map<std::string, ScriptField> m_ScriptFields;
		std::unordered_map<uint32_t, uint32_t*> m_PhysicsSensorMap;

		friend class Entity;
		friend class Scene;
		friend class SceneSerializer;
		friend class Client;

		friend class InspectorPanel;
	};
}
