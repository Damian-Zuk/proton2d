#pragma once

#include "Entity.h"

namespace proton {

	enum class ScriptFieldType { Float = 0 };

	struct ScriptField
	{
		ScriptFieldType Type = ScriptFieldType::Float;
		void* InstanceFieldValue = nullptr;
	};

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

		void RegisterFloatField(const std::string& name, float* field)
		{
			m_ScriptFields[name] = { ScriptFieldType::Float, (void*)field };
		}

		b2Body* GetBox2DRigidBody()
		{
			assert(HasComponent<RigidBodyComponent>());
			auto& id = GetComponent<IDComponent>();
			return m_Entity.m_Scene->GetBox2DRigidBody(id.ID);
		}

		Scene* GetScene()
		{
			return m_Entity.m_Scene;
		}

		// Register class members for serialization / editor view
		virtual void RegisterFields() {};
		virtual void OnCreate() {}
		virtual void OnDestroy() {}
		virtual void OnUpdate(float ts) {}

	protected:
		Entity m_Entity;
		
	private:
		std::unordered_map<std::string, ScriptField> m_ScriptFields;

		friend class Scene;
		friend class Inspector;
		friend class SceneSerializer;
	};
}