#pragma once

#include "proton/Scene/Entity.h"
#include "proton/Scene/ScriptFactory.h"

#define ENTITY_SCRIPT_CLASS(script_class) \
	static bool __FactoryRegistered; \
	static bool __FactoryRegisterClass() { \
		proton::ScriptFactory::Get().RegisterScript([&](proton::Entity entity) { \
			return entity.AddScript<script_class>(#script_class); \
		}, #script_class); \
		return true; \
	}

#define ENTITY_SCRIPT_IMPLEMENTATION(script_class) \
	bool script_class::__FactoryRegistered = script_class::__FactoryRegisterClass();

namespace proton {

	// FloatX types -> glm::vecX
	// IntX types -> glm::ivecX
	enum class ScriptFieldType { Float = 0, Float2, Float3, Float4, Int, Int2, Int3, Int4, Bool };

	struct ScriptField
	{
		ScriptFieldType Type = ScriptFieldType::Float;
		void* InstanceFieldValue = nullptr;
	};

	// When creating EntityScript derived class make sure
	// to enter ENTITY_SCRIPT_CLASS(script_class) macro inside class definition
	// and ENTITY_SCRIPT_IMPL(script_class) inside .cpp file.
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

		// Register class members for serialization and editor view in this method
		// Use RegisterField function
		virtual void RegisterFields() {};

		// Use glm::value_ptr for FloatX and IntX field types
		void RegisterField(ScriptFieldType type, const std::string& name, void* field)
		{
			m_ScriptFields[name] = { type, field };
		}

		Scene* GetScene() { return m_Entity.m_Scene; }

		void AddChild(Entity entity) { m_Entity.AddChildEntity(entity); }

		template<typename T, typename... Types>
		T& AddComponent(Types&& ...args)
		{ 
			return m_Entity.AddComponent<T>(std::forward<Types>(args)...);
		}

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

		Shared<Sprite> GetSprite()
		{
			return m_Entity.GetComponent<SpriteComponent>().Sprite;
		}

		// ******************************
		//  Rigidbody related methods
		// ******************************

		// Requires RigidbodyComponent
		b2Body* GetBox2DRigidbody() { return m_Entity.GetBox2DRigidbody(); }

		// Requires RigidbodyComponent
		glm::vec2 GetVelocity() { return m_Entity.GetVelocity(); }
		// Requires RigidbodyComponent
		void SetVelocity(float x_mps, float y_mps) { m_Entity.SetVelocity(x_mps, y_mps); }
		// Requires RigidbodyComponent
		void SetVelocityX(float mps) { m_Entity.SetVelocityX(mps); }
		// Requires RigidbodyComponent
		void SetVelocityY(float mps) { m_Entity.SetVelocityY(mps); }

		// Requires RigidbodyComponent
		void ApplyImpulse(const glm::vec2& impulse) { m_Entity.ApplyImpulse(impulse); }

	protected:
		Entity m_Entity;
		
	private:
		std::unordered_map<std::string, ScriptField> m_ScriptFields;

		friend class Scene;
		friend class Inspector;
		friend class SceneSerializer;
	};
}