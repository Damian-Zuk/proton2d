#pragma once

#include "proton/Scene/Entity.h"
#include "proton/Scene/ScriptFactory.h"

#define ENTITY_SCRIPT_CLASS(class_name) \
	static bool __FactoryRegistered; \
	static bool __FactoryRegisterClass() { \
		ScriptFactory::Get().RegisterScript( [&](Entity entity) { \
			return entity.AddScript<class_name>(#class_name); }, #class_name); \
		return true; \
	}

#define ENTITY_SCRIPT_IMPLEMENTATION(class_name) \
	bool class_name::__FactoryRegistered = class_name::__FactoryRegisterClass();

namespace proton {

	enum class ScriptFieldType { Float = 0, Float2, Float3, Float4, Int, Bool, String };

	struct ScriptField
	{
		ScriptFieldType Type = ScriptFieldType::Float;
		void* InstanceFieldValue = nullptr;
	};

	// When creating EntityScript derived class make sure
	// to enter ENTITY_SCRIPT_CLASS(class_name) macro inside class body
	// and ENTITY_SCRIPT_IMPL(class_name) inside .cpp file.
	// This will register class and allow Editor and SceneSerializer
	// to add script to entity based on class name string.
	// All registered classes are stored inside ScriptFactory class.
	class EntityScript
	{
	public:
		virtual ~EntityScript() = default;

		virtual void OnCreate() {}
		virtual void OnDestroy() {}
		virtual void OnUpdate(float ts) {}
		// Register class members for serialization and editor view here
		virtual void RegisterFields() {};

		Scene* GetScene() { return m_Entity.m_Scene; }

		template<typename T>
		T& GetComponent() { return m_Entity.GetComponent<T>(); }

		template<typename T>
		bool HasComponent() { return m_Entity.HasComponent<T>(); }

		template<typename T>
		void RemoveComponent() { m_Entity.RemoveComponent<T>(); }

		void DestroyEntity() { m_Entity.Destroy(); }

		TransformComponent& GetTransform()
		{
			return m_Entity.GetComponent<TransformComponent>();
		}

		SpriteComponent& GetSpriteRenderable()
		{
			return m_Entity.GetComponent<SpriteComponent>();
		}

		void RegisterField(ScriptFieldType type, const std::string& name, void* field)
		{
			m_ScriptFields[name] = { type, field };
		}

		b2Body* GetBox2DRigidbody() { return m_Entity.GetBox2DRigidbody(); }

		// Requires RigidbodyComponent
		void SetVelocity(float x_mps, float y_mps) { m_Entity.SetVelocity(x_mps, y_mps); }
		// Requires RigidbodyComponent
		void SetVelocityX(float mps) { m_Entity.SetVelocityX(mps); }
		// Requires RigidbodyComponent
		void SetVelocityY(float mps) { m_Entity.SetVelocityY(mps); }

		// Requires RigidbodyComponent
		glm::vec2 GetVelocity() { return m_Entity.GetVelocity(); }

		// Requires RigidbodyComponent
		void ApplyImpulse(const glm::vec2& impulse) { m_Entity.ApplyImpulse(impulse); }

		void AddChild(Entity entity) { m_Entity.AddChildEntity(entity); }

	protected:
		Entity m_Entity;
		
	private:
		std::unordered_map<std::string, ScriptField> m_ScriptFields;

		friend class Scene;
		friend class Inspector;
		friend class SceneSerializer;
	};
}