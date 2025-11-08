#pragma once
#include "Proton/Scripting/GameScript.h"
#include "Proton/Scene/Entity.h"

namespace proton {

	class EntityScript : public Entity, public Script
	{
	public:
		EntityScript();
		virtual ~EntityScript() = default;

		virtual bool OnCreate() override { return true; }
		virtual void OnDestroy() override {};
		virtual void OnUpdate(float ts) {};
		virtual void OnFixedUpdate(float ts) override {};
		virtual void OnPreInit() override {};
		virtual void OnEvent(Event& event) override {};
		virtual void OnImGuiRender() override {};

		// Physics
		void SetPhysicsSensor(uint32_t sensorType, const std::string& childEntityTagName);
		bool CheckSensor(uint32_t sensorType) const;
		uint32_t GetSensorContactCount(uint32_t sensorType) const;

	private:
		std::unordered_map<uint32_t, uint32_t*> m_PhysicsSensorMap;

		friend class Entity;
	};

}