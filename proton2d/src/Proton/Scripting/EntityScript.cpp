#include "ptpch.h"
#include "Proton/Scripting/EntityScript.h"
#include "Proton/Core/Application.h"
#include "Proton/Core/GameInstance.h"
#include "Proton/Network/NetworkManager.h"

namespace proton {

	void EntityScript::RegisterField(ScriptFieldType type, const std::string& name, void* field, bool showInEditor, bool networkSerialize)
	{
		m_ScriptFields[name] = { type, field, networkSerialize, showInEditor };
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

	void EntityScript::SetReplicatedField(const std::string& name, const std::function<void(Entity*)>& notifyFunction)
	{
		PT_CORE_ASSERT(m_ScriptFields.find(name) != m_ScriptFields.end(), "Script field not found");

		ScriptField* scriptField = &m_ScriptFields.at(name);
		SetReplicatedData(scriptField->InstanceFieldValue, GetFieldSize(scriptField->Type), notifyFunction);
	}

	static bool CompareByTag(const NetworkComponent::ReplicatedScript& a, const  NetworkComponent::ReplicatedScript& b) {
		return a.Script->GetTag() < b.Script->GetTag();
	}

	void EntityScript::SetReplicatedData(void* data, size_t size, const std::function<void(Entity*)>& notifyFunction)
	{
		if (!HasComponent<NetworkComponent>())
		{
			PT_CORE_ERROR("Entity: ({}, {}) missing NetworkComponent)", GetUUID(), GetTag());
			return;
		}

		auto& net = GetComponent<NetworkComponent>();
		auto& repScripts = net.ReplicatedScripts;

		net.ComponentsToReplicate.set(ComponentType_Script);

		auto scriptRepInfo = std::find_if(repScripts.begin(), repScripts.end(),
			[this](const auto& repInfo) { return repInfo.Script == this; });

		if (scriptRepInfo == repScripts.end())
		{
			NetworkComponent::ReplicatedScript newEntry;
			newEntry.Script = this;
			newEntry.ReplicatedFields.push_back({ data, size, 0, notifyFunction });
			
			auto insertPosition = std::lower_bound(repScripts.begin(), repScripts.end(), newEntry, CompareByTag);
			repScripts.insert(insertPosition, newEntry);
			return;
		}

		scriptRepInfo->ReplicatedFields.push_back({ data, size, 0, notifyFunction });
	}

	bool EntityScript::IsNetworkPhysicsSyncEnabled() const
	{
		Scene* scene = GetScene();
		auto& net = GetComponent<NetworkComponent>();
		return scene->IsPhysicsSimulated() && net.SyncParams.SyncMethod == NetSyncMethod::NetworkRigidbody;
	}

	bool EntityScript::IsRunningServer() const
	{
		NetMode netMode = GetScene()->GetGameInstance()->GetNetworkManager()->GetNetMode();
		return netMode == NetMode::ListenServer || netMode == NetMode::DedicatedServer;
	}

	bool EntityScript::IsRunningClient() const
	{
		return !HasAuthority();
	}

	bool EntityScript::HasAuthority() const
	{
		NetMode netMode = GetScene()->GetGameInstance()->GetNetworkManager()->GetNetMode();
		return netMode != NetMode::Client;
	}

	const size_t EntityScript::GetFieldSize(ScriptFieldType type)
	{
		switch (type)
		{
		case ScriptFieldType::Float:
			return sizeof(float);
		case ScriptFieldType::Float2:
			return sizeof(glm::vec2);
		case ScriptFieldType::Float3:
			return sizeof(glm::vec3);
		case ScriptFieldType::Float4:
			return sizeof(glm::vec4);
		case ScriptFieldType::Int:
			return sizeof(int);
		case ScriptFieldType::Int2:
			return sizeof(glm::ivec2);
		case ScriptFieldType::Int3:
			return sizeof(glm::ivec3);
		case ScriptFieldType::Int4:
			return sizeof(glm::ivec4);
		case ScriptFieldType::Bool:
			return sizeof(bool);
		default:
			PT_CORE_ASSERT("Unexpected value type!");
			return 0;
		}
	}

	void EntityScript::SetFieldValueData(const std::string& fieldName, void* valuePtr)
	{
		ScriptField& field = m_ScriptFields[fieldName];

		switch (field.Type)
		{
		case ScriptFieldType::Float:
			*(float*)field.InstanceFieldValue = *(float*)valuePtr;
			break;
		case ScriptFieldType::Float2:
			*(glm::vec2*)field.InstanceFieldValue = *(glm::vec2*)valuePtr;
			break;
		case ScriptFieldType::Float3:
			*(glm::vec3*)field.InstanceFieldValue = *(glm::vec3*)valuePtr;
			break;
		case ScriptFieldType::Float4:
			*(glm::vec4*)field.InstanceFieldValue = *(glm::vec4*)valuePtr;
			break;
		case ScriptFieldType::Int:
			*(int*)field.InstanceFieldValue = *(int*)valuePtr;
			break;
		case ScriptFieldType::Int2:
			*(glm::ivec2*)field.InstanceFieldValue = *(glm::ivec2*)valuePtr;
			break;
		case ScriptFieldType::Int3:
			*(glm::ivec3*)field.InstanceFieldValue = *(glm::ivec3*)valuePtr;
			break;
		case ScriptFieldType::Int4:
			*(glm::ivec4*)field.InstanceFieldValue = *(glm::ivec4*)valuePtr;
			break;
		case ScriptFieldType::Bool:
			*(bool*)field.InstanceFieldValue = *(bool*)valuePtr;
			break;
		default:
			PT_CORE_ASSERT("Unexpected value type!");
			break;
		}
	}

}
