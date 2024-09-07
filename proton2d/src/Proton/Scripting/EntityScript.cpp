#include "ptpch.h"
#include "Proton/Scripting/EntityScript.h"
#include "Proton/Core/Application.h"
#include "Proton/Core/GameInstance.h"
#include "Proton/Network/NetworkManager.h"
#include "Proton/Core/Input.h"

#ifdef PT_EDITOR
	#include "Proton/Editor/EditorLayer.h"
	#include <imgui.h>
#endif

namespace proton {

	void EntityScript::RegisterField(ScriptFieldType type, const std::string& name, void* field, size_t size, bool showInEditor)
	{
		m_ScriptFields[name] = { type, field, size };
	#ifdef PT_EDITOR
		m_EditorScriptField[name] = { showInEditor };
	#endif
	}

	void EntityScript::SetPhysicsSensor(uint32_t sensorType, const std::string& childEntityTagName)
	{
		Entity child = FindChildByTag(childEntityTagName);

		if (!child)
		{
			PT_CORE_ASSERT(false, "Child entity not found.");
			return;
		}

		if (child.HasComponent<BoxColliderComponent>())
		{
			auto& component = child.GetComponent<BoxColliderComponent>();
			m_PhysicsSensorMap[sensorType] = &component.ContactCallback.ContactCount;
			return;
		}

		if (child.HasComponent<CircleColliderComponent>())
		{
			auto& component = child.GetComponent<CircleColliderComponent>();
			m_PhysicsSensorMap[sensorType] = &component.ContactCallback.ContactCount;
			return;
		}

		PT_CORE_ASSERT(false, "Sensor has no collider component!");
	}

	uint32_t EntityScript::GetSensorContactCount(uint32_t sensorType) const
	{
		bool valid = m_PhysicsSensorMap.find(sensorType) != m_PhysicsSensorMap.end();
		PT_CORE_ASSERT(valid, "Sensor type not set!");
		return *m_PhysicsSensorMap.at(sensorType);
	}

	bool EntityScript::CheckSensor(uint32_t sensorType) const
	{
		bool valid = m_PhysicsSensorMap.find(sensorType) != m_PhysicsSensorMap.end();
		PT_CORE_ASSERT(valid, "Sensor type not set!");
		return *m_PhysicsSensorMap.at(sensorType) > 0;
	}

	SceneManager* EntityScript::GetSceneManager() const
	{
		return GetScene()->GetGameInstance()->GetSceneManager();
	}

	bool EntityScript::IsKeyPressed(KeyCode key) const
	{
	#ifdef PT_EDITOR
		GameInstance* focusedInstance = EditorLayer::Get()->GetFocusedGameInstance();
		if (GetScene() != focusedInstance->GetActiveScene())
			return false;
		
		const auto& io = ImGui::GetIO();
		if (io.WantTextInput)
			return false;
	#endif
		return Input::IsKeyPressed(key);
	}

	bool EntityScript::IsMouseButtonPressed(MouseCode button) const
	{
	#ifdef PT_EDITOR
		GameInstance* focusedInstance = EditorLayer::Get()->GetFocusedGameInstance();
		if (GetScene() != focusedInstance->GetActiveScene())
			return false;

		const auto& io = ImGui::GetIO();
		if (io.WantCaptureMouse)
			return false;
	#endif
		return Input::IsMouseButtonPressed(button);
	}

	// ------------------------------- Networking -------------------------------

	NetworkManager* EntityScript::GetNetworkManager() const
	{
		return GetScene()->GetGameInstance()->GetNetworkManager();
	}

	bool EntityScript::IsNetModeServer() const
	{
		NetworkManager* netManager = GetNetworkManager();
		return netManager->IsNetModeServer() && netManager->IsNetworkActive();
	}

	bool EntityScript::IsNetModeClient() const
	{
		NetworkManager* netManager = GetNetworkManager();
		return netManager->IsNetModeClient() && netManager->IsNetworkActive();
	}

	void EntityScript::Client_SendCustomMessage(const NetworkStreamWriterDelegate& delegate) const
	{
		GetNetworkManager()->Client_SendCustomMessage(delegate);
	}

	void EntityScript::Server_SendCustomMessage(ClientID clientID, const NetworkStreamWriterDelegate& delegate) const
	{
		GetNetworkManager()->Server_SendCustomMessage(clientID, delegate);
	}

	bool EntityScript::HasAuthority() const
	{
		NetMode netMode = GetNetworkManager()->GetNetMode();
		return netMode != NetMode::Client;
	}

	using ReplicatedScript = NetworkComponent::ReplicatedScript;
	using ReplicatedField = ReplicatedScript::ReplicatedField;

	static bool CompareByClassName(const ReplicatedScript& a, const  ReplicatedScript& b) {
		return a.Script->GetScriptClassName() < b.Script->GetScriptClassName();
	}

	void EntityScript::SetReplicatedData(void* data, size_t size, const std::function<void()>& notifyFunction)
	{
		SetReplicatedField(ScriptField{ ScriptFieldType::Data, data, size }, notifyFunction);
	}

	void EntityScript::SetReplicatedField(const std::string& fieldName, const std::function<void()>& notifyFunction)
	{
		SetReplicatedField(m_ScriptFields[fieldName], notifyFunction);
	}

	void EntityScript::SetReplicatedField(const ScriptField& field, const std::function<void()>& notifyFunction)
	{
		if (!HasComponent<NetworkComponent>())
		{
			PT_CORE_ERROR("Entity: ({}, {}) missing NetworkComponent)", GetUUID(), GetTag());
			return;
		}

		auto& net = GetComponent<NetworkComponent>();
		auto& repScripts = net.ReplicatedScripts;

		auto scriptRepInfo = std::find_if(repScripts.begin(), repScripts.end(),
			[this](const auto& repInfo) { return repInfo.Script == this; });

		if (scriptRepInfo == repScripts.end())
		{
			ReplicatedScript newEntry;
			newEntry.Script = this;
			newEntry.ReplicatedFields.push_back(ReplicatedField{ field, notifyFunction });

			auto insertPosition = std::lower_bound(repScripts.begin(), repScripts.end(), newEntry, CompareByClassName);
			repScripts.insert(insertPosition, newEntry);
			return;
		}

		scriptRepInfo->ReplicatedFields.push_back(ReplicatedField{ field, notifyFunction });
	}

	void EntityScript::SetFieldValueData(const std::string& fieldName, void* valuePtr)
	{
		ScriptField& field = m_ScriptFields[fieldName];

		switch (field.Type)
		{
		case ScriptFieldType::Float:
			*(float*)field.ValuePtr = *(float*)valuePtr;
			break;
		case ScriptFieldType::Float2:
			*(glm::vec2*)field.ValuePtr = *(glm::vec2*)valuePtr;
			break;
		case ScriptFieldType::Float3:
			*(glm::vec3*)field.ValuePtr = *(glm::vec3*)valuePtr;
			break;
		case ScriptFieldType::Float4:
			*(glm::vec4*)field.ValuePtr = *(glm::vec4*)valuePtr;
			break;

		case ScriptFieldType::Int:
			*(int*)field.ValuePtr = *(int*)valuePtr;
			break;
		case ScriptFieldType::Int2:
			*(glm::ivec2*)field.ValuePtr = *(glm::ivec2*)valuePtr;
			break;
		case ScriptFieldType::Int3:
			*(glm::ivec3*)field.ValuePtr = *(glm::ivec3*)valuePtr;
			break;
		case ScriptFieldType::Int4:
			*(glm::ivec4*)field.ValuePtr = *(glm::ivec4*)valuePtr;
			break;

		case ScriptFieldType::Bool:
			*(bool*)field.ValuePtr = *(bool*)valuePtr;
			break;

		case ScriptFieldType::String:
			*(std::string*)field.ValuePtr = *(std::string*)valuePtr;
			break;

		case ScriptFieldType::Data:
			memcpy(valuePtr, field.ValuePtr, field.Size);
			break;

		default:
			PT_CORE_ASSERT("Unexpected value type!");
			break;
		}
	}

}
