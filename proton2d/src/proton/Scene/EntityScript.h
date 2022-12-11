#pragma once

#include "Entity.h"

namespace proton {

	class EntityScript
	{
	public:
		virtual ~EntityScript() = default;

		template<typename T>
		T& GetComponent() 
		{ 
			return m_Entity.GetComponent<T>(); 
		}

		template<typename T>
		bool HasComponent()
		{
			return m_Entity.HasComponent<T>();
		}

		template<typename T>
		void RemoveComponent()
		{
			m_Entity.RemoveComponent<T>();
		}

		void DestroyEntity()
		{
			m_Entity.Destroy();
		}

		virtual void OnCreate() {}
		virtual void OnDestroy() {}
		virtual void OnUpdate(float ts) {}

	private:
		Entity m_Entity;

		friend class Scene;
	};
}