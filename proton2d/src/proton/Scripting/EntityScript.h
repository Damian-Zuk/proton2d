#pragma once
#include "Proton/Scene/Entity.h"
#include "Proton/Scripting/ScriptFactory.h"
#include "Proton/Core/KeyCodes.h"

// This macro registers script class inside ScriptFactory.
// Script must inherit from EntityScript base class.
#define ENTITY_SCRIPT_CLASS(script_class) \
	static inline const std::string __ScriptClassName = #script_class; \
	virtual const std::string& GetScriptClassName() override { return __ScriptClassName; } \
	static inline const bool __RegisteredInFactory = \
		proton::ScriptFactory::Get().RegisterScript([&](proton::Entity entity) { \
			return entity.AddScript<script_class>(); \
		}, #script_class); \

#define REGISTER_FIELD(type, field) \
	RegisterField(ScriptFieldType::type, #field, &field, sizeof(field), true);

#define REGISTER_FIELD_NO_EDIT(type, field) \
	RegisterField(ScriptFieldType::type, #field, &field, sizeof(field), false);

#define REPLICATED_FIELD(field, ...) \
	SetReplicatedField(#field, __VA_ARGS__)

#define REPLICATED_DATA(data, ...) \
	SetReplicatedData(&data, sizeof(data), __VA_ARGS__);


namespace proton {

	// Forward declaration
	class SceneManager;
	class NetworkManager;

	// Base class for entity scripts.
	// - Use ENTITY_SCRIPT_CLASS for script class registration.
	// - Implement OnCreate, OnDestroy, OnUpdate for Entity behavior.
	// - Use OnRegisterFields method to register fields (variables) for serialization / editor view.
	class EntityScript : public Entity
	{
	public:
		// ENTITY_SCRIPT_CLASS(EntityScript)
		virtual const std::string& GetScriptClassName() = 0; // Defined by ENTITY_SCRIPT_CLASS macro

		virtual ~EntityScript() = default;

		virtual bool OnCreate() { return true; }
		virtual void OnDestroy() {}
		virtual void OnUpdate(float ts) {}
		virtual void OnFixedUpdate(float ts) {}

		// Register your fields (variables) in this method.
		// Supported variable types are contained in ScriptFieldType enum.
		// Call REGISTER_FIELD, REPLICATED_FIELD, REPLICATED_DATA macros here.
		virtual void OnRegisterFields() {}

		// Draw your custom ImGui Editor interface here.
		// The ImGui elements will be displayed within the script node in the Inspector Panel.
		// IMPORTANT: Ensure that your ImGui code is enclosed within #ifdef PT_EDITOR preprocessor directives.
		virtual void OnImGuiRender() {}

		// Scene
		SceneManager* GetSceneManager() const;
		NetworkManager* GetNetworkManager() const;
		
		// Physics
		void SetPhysicsSensor(uint32_t sensorType, const std::string& childEntityTagName);
		bool CheckSensor(uint32_t sensorType) const;
		uint32_t GetSensorContactCount(uint32_t sensorType) const;

		// Input
		bool IsKeyPressed(KeyCode key) const;
		bool IsMouseButtonPressed(MouseCode button) const;
		
		// -------------------------- Networking  --------------------------
		bool HasAuthority() const;
		bool IsNetModeServer() const;
		bool IsNetModeClient() const;

		void Client_SendCustomMessage(const NetworkStreamWriterDelegate& delegate) const;
		void Server_SendCustomMessage(ClientID clientID, const NetworkStreamWriterDelegate& delegate) const;

		// Use REPLICATED_FIELD and REPLICATED_DATA macros instead of directly calling these methods
		void SetReplicatedField(const std::string& fieldName, const std::function<void()>& notifyFunction = nullptr);
		void SetReplicatedField(const ScriptField& field, const std::function<void()>& notifyFunction = nullptr);
		void SetReplicatedData(void* data, size_t size, const std::function<void()>& notifyFunction = nullptr);

		// Supported variable types are listed inside ScriptFieldType enum.
		void RegisterField(ScriptFieldType type, const std::string& name, void* valuePtr, size_t size, bool showInEditor = true);

	private:
		void SetFieldValueData(const std::string& fieldName, void* value);
		
	private:
		bool m_Initialized = false;
		bool m_FailedToInitialize = false;

		std::unordered_map<std::string, ScriptField> m_ScriptFields;
		std::unordered_map<uint32_t, uint32_t*> m_PhysicsSensorMap;

	#ifdef PT_EDITOR
		std::unordered_map<std::string, EditorScriptField> m_EditorScriptField;
	#endif

		friend class Entity;
		friend class Scene;
		friend class SceneSerializer;
		friend class Client;

		friend class InspectorPanel;
	};
}
