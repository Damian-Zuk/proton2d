#include "ptpch.h"
#include "Proton/Scripting/Script.h"
#include "Proton/Scripting/EntityScript.h"
#include "Proton/Core/Application.h"
#include "Proton/Network/NetworkManager.h"

#ifdef PT_EDITOR
#include <Proton/Editor/EditorLayer.h>
#endif

#include <imgui.h>

namespace proton {

	//Script::Script(ScriptType type)
	//	: m_ScriptType(type)
	//{
	//}

	void Script::RegisterField(ScriptFieldType type, const std::string& name, void* valuePtr, size_t size, bool showInEditor)
	{
		PT_CORE_VERIFY(size == GetScriptFieldSize(type));
		m_ScriptFields[name] = { valuePtr, type, size, showInEditor };
	}

	bool Script::IsKeyPressed(KeyCode key) const
	{
		#ifdef PT_EDITOR
		if (EditorLayer::GetFocusedGameInstance() != m_GameInstance)
			return false;

		const auto& io = ImGui::GetIO();
		if (io.WantTextInput)
			return false;
		#endif
		return Input::IsKeyPressed(key);
	}

	bool Script::IsMouseButtonPressed(MouseCode button) const
	{
		#ifdef PT_EDITOR
		if (EditorLayer::GetFocusedGameInstance() != m_GameInstance)
			return false;

		const auto& io = ImGui::GetIO();
		if (io.WantCaptureMouse)
			return false;
		#endif
		return Input::IsMouseButtonPressed(button);
	}

	SceneManager* Script::GetSceneManager() const
	{
		return m_GameInstance->GetSceneManager();
	}

	NetworkManager* Script::GetNetworkManager() const
	{
		return m_GameInstance->GetNetworkManager();
	}

	bool Script::IsNetModeServer() const
	{
		NetworkManager* net = GetNetworkManager();
		return net->IsNetModeServer() && net->IsNetworkActive();
	}

	bool Script::IsNetModeClient() const
	{
		NetworkManager* net = GetNetworkManager();
		return net->IsNetModeClient() && net->IsNetworkActive();
	}

	void Script::Client_SendCustomMessage(const NetworkStreamWriterDelegate& delegate) const
	{
		GetNetworkManager()->Client_SendCustomMessage(delegate);
	}

	void Script::Server_SendCustomMessage(ClientID clientID, const NetworkStreamWriterDelegate& delegate) const
	{
		GetNetworkManager()->Server_SendCustomMessage(clientID, delegate);
	}

	bool Script::HasAuthority() const
	{
		NetMode netMode = GetNetworkManager()->GetNetMode();
		return netMode != NetMode::Client;
	}

	void Script::SetReplicatedData(void* data, size_t size, const NotifyFunction& notifyFunction)
	{
		SetReplicatedField(ScriptField{ data, ScriptFieldType::Data, size, false }, notifyFunction);
	}

	void Script::SetReplicatedField(const std::string& fieldName, const NotifyFunction& notifyFunction)
	{
		SetReplicatedField(m_ScriptFields.at(fieldName), notifyFunction);
	}

	static bool CompareByClassName(const ReplicatedScript& a, const  ReplicatedScript& b) {
		return a.Script->GetScriptClassName() < b.Script->GetScriptClassName();
	}

	void Script::SetReplicatedField(const ScriptField& field, const NotifyFunction& notifyFunction)
	{
		ReplicatedScripts* repScripts = nullptr;

		if (m_ScriptType == ScriptType::EntityScript)
		{
			EntityScript* script = dynamic_cast<EntityScript*>(this);
			auto& net = script->GetComponent<NetworkComponent>();
			repScripts = &net.ReplicatedScripts;
		}

		PT_CORE_ASSERT(repScripts);
		if (!repScripts) 
			return;

		auto scriptRepInfo = std::find_if(repScripts->begin(), repScripts->end(),
			[this](const auto& repInfo) { return repInfo.Script == this; }
		);

		if (scriptRepInfo == repScripts->end())
		{
			ReplicatedScript newEntry;
			newEntry.Script = this;
			newEntry.ReplicatedFields.push_back(ReplicatedField{ field, notifyFunction });

			auto insertPosition = std::lower_bound(repScripts->begin(), repScripts->end(), newEntry, CompareByClassName);
			repScripts->insert(insertPosition, newEntry);
			return;
		}

		scriptRepInfo->ReplicatedFields.push_back(ReplicatedField{ field, notifyFunction });
	}

	void Script::SetFieldValueData(const std::string& fieldName, void* valuePtr)
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
			//memcpy(valuePtr, field.ValuePtr, field.Size);
			break;

		default:
			PT_CORE_ASSERT("Unexpected value type!");
			break;
		}
	}

}