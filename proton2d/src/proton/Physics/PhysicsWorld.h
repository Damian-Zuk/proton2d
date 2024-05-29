#pragma once
#include "Proton/Core/UUID.h"
#include "Proton/Scene/Entity.h"

#include <queue>

class b2Body;

namespace proton {

	class Scene;

	enum class JointType : uint8_t
	{
		Revolute = 0 // Currently only revolute joints supported
	};

	class PhysicsContactListener : public b2ContactListener
	{
	public:
		PhysicsContactListener(Scene* scene);
		virtual void BeginContact(b2Contact* contact) override;
		virtual void EndContact(b2Contact* contact) override;
		virtual void PreSolve(b2Contact* contact, const b2Manifold* oldManifold) override;
		virtual void PostSolve(b2Contact* contact, const b2ContactImpulse* impulse) override;

	private:
		Scene* m_Scene = nullptr;
	};

	class PhysicsWorld
	{
	public:
		PhysicsWorld(Scene* context);
		virtual ~PhysicsWorld();

		b2Body* GetRuntimeBody(UUID id);
		b2Body* CreateRuntimeBody(Entity entity);
		void DestroyRuntimeBody(UUID id);

		bool IsInitialized() { return m_World != nullptr; }

	private:
		void BuildWorld();
		void DestroyWorld();

		void ProcessCreatedEntities();
		void Update(float ts);

		void CreateJoints();

		void AddFixtureToRuntimeBody(Entity entity, b2Body* body = nullptr, bool attachToParent = false);
	
	private:
		b2World* m_World = nullptr;
		Scene* m_Scene = nullptr;

		std::unordered_map<UUID, b2Body*> m_RuntimeBodies;
		std::vector<Unique<Entity>> m_FixtureUserData;
		std::vector<Entity> m_EntitiesToInitialize;

		std::vector<b2Joint*> m_Joints;
		
		struct JointInfo
		{
			UUID EntityA_UUID;
			UUID EntityB_UUID;
			JointType Type;
		};
		std::queue<JointInfo> m_JointsCreateQueue;

		int m_PhysicsVelocityIterations = 5;
		int m_PhysicsPositionIterations = 5;
		float m_Gravity = 9.8f;
		
		PhysicsContactListener m_ContactListener;

		friend class Scene;
		friend class SceneSerializer;
		friend class Entity;
		friend class NetClientTransformSyncSystem;

		friend class EditorLayer;
		friend class ToolbarPanel;
		friend class InspectorPanel;
	};

}
