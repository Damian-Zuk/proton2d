#include "ptpch.h"
#include "Proton/Scripting/EntityScript.h"

namespace proton {

	EntityScript::EntityScript()
		: Script(ScriptType::EntityScript)
	{
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

}
