#include "ptpch.h"
#include "Proton/Scripting/EntityScript.h"
#include "Proton/Core/Application.h"
#include "Proton/Core/GameInstance.h"
#include "Proton/Network/Common/NetworkManager.h"

namespace proton {

	void EntityScript::RegisterField(ScriptFieldType type, const std::string& name, void* field, bool showInEditor)
	{
		m_ScriptFields[name] = { type, field, showInEditor };
	}

	void EntityScript::SetPhysicsSensor(uint32_t sensorType, const std::string& childEntityTagName)
	{
		Entity child = FindChildByTag(childEntityTagName);

		if (!child)
		{
			PT_CORE_ASSERT(child, "Child entity '{}' not found.", childTagName);
			return;
		}

		if (child.HasComponent<BoxColliderComponent>())
		{
			auto& component = child.GetComponent<BoxColliderComponent>();
			m_PhysicsSensorMap[sensorType] = &component.ContactCallback.ContactCount;
		}

		if (child.HasComponent<CircleColliderComponent>())
		{
			auto& component = child.GetComponent<CircleColliderComponent>();
			m_PhysicsSensorMap[sensorType] = &component.ContactCallback.ContactCount;
		}

		PT_CORE_ASSERT(false, "Sensor has no collider component! sensor_type={}, entity={}", sensorType, childEntityTagName);
	}

	uint32_t EntityScript::GetSensorContactCount(uint32_t sensorType) const
	{
		bool valid = m_PhysicsSensorMap.find(sensorType) != m_PhysicsSensorMap.end();
		PT_CORE_ASSERT(valid, "Sensor type {} not set!", sensorType);
		return *m_PhysicsSensorMap.at(sensorType);
	}

	bool EntityScript::CheckSensor(uint32_t sensorType) const
	{
		bool valid = m_PhysicsSensorMap.find(sensorType) != m_PhysicsSensorMap.end();
		PT_CORE_ASSERT(valid, "Sensor type {} not set!", sensorType);
		return *m_PhysicsSensorMap.at(sensorType) > 0;
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
			PT_ASSERT("Unexpected value type!");
			break;
		}
	}

	SceneManager* EntityScript::GetSceneManager() const
	{
		return GetScene()->GetOwningGameInstance()->GetSceneManager();
	}

	bool EntityScript::IsRunningServer() const
	{
		NetMode netMode = GetScene()->GetOwningGameInstance()->GetNetworkManager()->GetNetMode();
		return netMode == NetMode::ListenServer || netMode == NetMode::DedicatedServer;
	}

	bool EntityScript::IsRunningClient() const
	{
		return !HasAuthority();
	}

	bool EntityScript::HasAuthority() const
	{
		NetMode netMode = GetScene()->GetOwningGameInstance()->GetNetworkManager()->GetNetMode();
		return netMode != NetMode::Client;
	}

}
