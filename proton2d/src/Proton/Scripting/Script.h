#pragma once
#include "Proton/Core/GameInstance.h"
#include "Proton/Core/Input.h"
#include "Proton/Events/Event.h"
#include "Proton/Scripting/ScriptField.h"
#include "Proton/Network/ReplicatedScript.h"
#include "Proton/Network/NetworkManager.h"

namespace proton {

	class GameInstance;

	enum class ScriptType : uint16_t
	{
		None = 0,
		AppScript,
		GameScript,
		EntityScript
	};

	enum class ScriptStatus : uint16_t
	{
		Uninitalized = 0,
		Initialized,
		Failure
	};

	// Native Script base class
	class Script
	{
	public:
		Script(ScriptType type);
		virtual ~Script() = default;

		virtual void OnPreInit() = 0;
		virtual bool OnCreate() = 0;
		virtual void OnDestroy() = 0;
		virtual void OnUpdate(float ts) = 0;
		virtual void OnFixedUpdate(float ts) = 0;
		virtual void OnEvent(Event& event) = 0;
		virtual void OnImGuiRender() = 0;

		template<typename TScript>
		TScript* As()
		{
			return dynamic_cast<TScript*>(this);
		}

		SceneManager* GetSceneManager() const;
		NetworkManager* GetNetworkManager() const;

		void RegisterField(ScriptFieldType type, const std::string& name, void* valuePtr, size_t size, bool showInEditor = true);

		// Input
		bool IsKeyPressed(KeyCode key) const;
		bool IsMouseButtonPressed(MouseCode button) const;

		// Network
		void SetReplicatedField(const std::string& fieldName, const NotifyFunction& notifyFunc = nullptr);
		void SetReplicatedField(const ScriptField& field, const NotifyFunction& notifyFunc = nullptr);

		//template<typename T>
		void SetReplicatedData(void* data, size_t size, const NotifyFunction& notifyFunc = nullptr);

		bool HasAuthority() const;
		bool IsNetModeServer() const;
		bool IsNetModeClient() const;

		void Client_SendCustomMessage(const NetworkStreamWriterDelegate& delegate) const;
		void Server_SendCustomMessage(ClientID clientID, const NetworkStreamWriterDelegate& delegate) const;

		virtual const std::string& GetScriptClassName() = 0; // Implemented by `_SCRIPT_GENERATED_BODY` macro.
	
	private:
		void SetFieldValueData(const std::string& fieldName, void* value);

	protected:
		ScriptType m_ScriptType = ScriptType::None;

	private:
		GameInstance* m_GameInstance = nullptr;
		ScriptStatus m_Status = ScriptStatus::Uninitalized;
		std::unordered_map<std::string, ScriptField> m_ScriptFields;

		friend class ScriptFactory;
		friend class GameInstance;
		friend class Scene;
		friend class Entity;
		friend class ScriptSerializer;
		friend class SceneSerializer;

		friend class InspectorPanel;
		friend class ScriptEditWidget;
	};

}

#define REGISTER_FIELD(type, field) \
	RegisterField(proton::ScriptFieldType::type, #field, &field, sizeof(field), true);

#define REGISTER_FIELD_NO_EDIT(type, field) \
	RegisterField(proton::ScriptFieldType::type, #field, &field, sizeof(field), false);

#define REPLICATED_FIELD(field, ...) \
	SetReplicatedField(#field, __VA_ARGS__)

#define REPLICATED_DATA(data, ...) \
	static_assert(std::is_trivially_copyable_v<std::decay_t<decltype(data)>>, \
		"Replicated data must be trivially copyable."); \
	SetReplicatedData(&data, sizeof(data), __VA_ARGS__);
