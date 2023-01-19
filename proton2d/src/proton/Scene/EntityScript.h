#pragma once

#include "proton/Scene/Entity.h"
#include "proton/Scene/ScriptFactory.h"

// Macro that registers script class inside ScriptFactory
#define ENTITY_SCRIPT_CLASS(script_class) \
	static inline const char __ScriptClassName[] = #script_class; \
	static inline const bool __RegisteredInFactory = \
		proton::ScriptFactory::Get().RegisterScript([&](proton::Entity entity) { \
			return entity.AddScript<script_class>(); \
		}, #script_class);

namespace proton {

	// FloatX types are glm::vecX, IntX types are glm::ivecX
	enum class ScriptFieldType { Float = 0, Float2, Float3, Float4, Int, Int2, Int3, Int4, Bool };

	struct ScriptField
	{
		ScriptFieldType Type = ScriptFieldType::Float;
		void* InstanceFieldValue = nullptr;
	};

	// When creating EntityScript derived class make sure
	// to add ENTITY_SCRIPT_CLASS(script_class) macro inside class declaration
	// This will register class and allow Editor and SceneSerializer
	// to add script to entity based on its class name provided as string.
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
		virtual void OnRegisterFields() {}

		// Use glm::value_ptr for FloatX and IntX field types
		void RegisterField(ScriptFieldType type, const std::string& name, void* field)
		{
			m_ScriptFields[name] = { type, field };
		}

		// ******************************
		//  Entity methods
		// ******************************

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

		Sprite& GetSprite()
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
		bool m_Initialized = false;
		std::unordered_map<std::string, ScriptField> m_ScriptFields;

		friend class Scene;
		friend class Inspector;
		friend class SceneSerializer;
	};
}