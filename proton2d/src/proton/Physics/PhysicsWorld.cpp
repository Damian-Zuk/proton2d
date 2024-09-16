#include "ptpch.h"
#include "Proton/Physics/PhysicsWorld.h"
#include "Proton/Network/NetworkManager.h"

#include <box2d/b2_world.h>
#include <box2d/b2_body.h>
#include <box2d/b2_fixture.h>
#include <box2d/b2_polygon_shape.h>
#include "box2d/b2_circle_shape.h"
#include <box2d/b2_contact.h>
#include <box2d/b2_revolute_joint.h>
#include <box2d/b2_wheel_joint.h>

namespace proton {

	PhysicsWorld::PhysicsWorld(Scene* context)
		: m_ContactListener(context), m_Scene(context)
	{
	}

	PhysicsWorld::~PhysicsWorld()
	{
		DestroyWorld();
	}

	b2Body* PhysicsWorld::GetRuntimeBody(UUID id)
	{
		PT_CORE_ASSERT(m_RuntimeBodies.find(id) != m_RuntimeBodies.end(), "Physics runtime body not found!");
		return m_RuntimeBodies.at(id);
	}

	void PhysicsWorld::DestroyRuntimeBody(UUID id)
	{
		m_World->DestroyBody(GetRuntimeBody(id));
		m_RuntimeBodies.erase(id);
	}

	b2Body* PhysicsWorld::CreateRuntimeBody(Entity entity)
	{
		UUID uuid = entity.GetUUID();
		if (m_RuntimeBodies.find(uuid) != m_RuntimeBodies.end())
		{
			// PT_CORE_ASSERT(false, "Runtime body already exists!");
			PT_CORE_WARN("Runtime body already exists! (tag={}, uuid={})", entity.GetTag(), uuid);
			return nullptr;
		}

		auto& transform = entity.GetComponent<TransformComponent>();
		auto& rb = entity.GetComponent<RigidbodyComponent>();

		b2BodyDef bodyDef;
		bodyDef.type = rb.Type;
		bodyDef.position.Set(transform.WorldPosition.x, transform.WorldPosition.y);
		bodyDef.angle = glm::radians(transform.Rotation);
		bodyDef.linearDamping = rb.LinearDamping;
		bodyDef.angularDamping = rb.AngularDamping;
		bodyDef.gravityScale = rb.GravityScale;
		bodyDef.bullet = rb.IsBullet;

		b2Body* body = m_World->CreateBody(&bodyDef);
		body->SetFixedRotation(rb.FixedRotation);
		m_RuntimeBodies[uuid] = body;
		rb.RuntimeBody = body;

		AddFixtureToRuntimeBody(entity, body);

		auto& hierarchy = entity.GetComponent<RelationshipComponent>();
		if (rb.AttachToParent && hierarchy.Parent != entt::null)
		{
			Entity parent(hierarchy.Parent, m_Scene);
			m_JointsCreateQueue.push({ uuid, parent.GetUUID(), JointType::Revolute});
		}
		
		return body;
	}

	void PhysicsWorld::AddFixtureToRuntimeBody(Entity entity, b2Body* body, bool attachToParent)
	{
		if (!entity.HasAnyComponent<BoxColliderComponent, CircleColliderComponent>())
			return;

		auto& transform = entity.GetComponent<TransformComponent>();
		auto& uuid = entity.GetComponent<IDComponent>().ID;

		b2FixtureDef fixtureDef;

		// Store Entity in fixture data (for collisions and contacts)
		m_FixtureUserData.push_back(MakeUnique<Entity>(entity.m_Handle, m_Scene));
		fixtureDef.userData.pointer = (uintptr_t)(m_FixtureUserData.back().get());

		if (!body)
		{
			PT_CORE_ASSERT(entity.HasComponent<RigidbodyComponent>(), "Entity has no RigidbodyComponent");
			body = GetRuntimeBody(entity.GetUUID());
		}

		// BoxColliderComponent
		if (entity.HasComponent<BoxColliderComponent>())
		{
			auto& bc = entity.GetComponent<BoxColliderComponent>();

			b2Vec2 offset = { bc.Offset.x, bc.Offset.y };
			if (bc.AttachToParent)
			{
				offset.x += transform.LocalPosition.x;
				offset.y += transform.LocalPosition.y;
			}

			float hx = bc.Size.x * transform.Scale.x / 2.0f;
			float hy = bc.Size.y * transform.Scale.y / 2.0f;
			
			b2PolygonShape shape;
			shape.SetAsBox(hx, hy, offset, bc.AttachToParent ? glm::radians(transform.Rotation) : 0.0f);

			fixtureDef.shape = &shape;
			fixtureDef.isSensor = bc.IsSensor;
			fixtureDef.filter = bc.Filter;

			fixtureDef.friction = bc.Material.Friction;
			fixtureDef.restitution = bc.Material.Restitution;
			fixtureDef.restitutionThreshold = bc.Material.RestitutionThreshold;
			fixtureDef.density = bc.Material.Density;

			bc.RuntimeFixture = body->CreateFixture(&fixtureDef);
		}

		// CircleColliderComponent
		if (entity.HasComponent<CircleColliderComponent>())
		{
			auto& cc = entity.GetComponent<CircleColliderComponent>();

			b2CircleShape shape;
			if (cc.AttachToParent)
				shape.m_p.Set(cc.Offset.x + transform.LocalPosition.x, cc.Offset.y + transform.LocalPosition.y);
			else
				shape.m_p.Set(cc.Offset.x, cc.Offset.y);

			shape.m_radius = transform.Scale.x / 2.0f * cc.Radius;
			fixtureDef.shape = &shape;
			fixtureDef.isSensor = cc.IsSensor;

			fixtureDef.friction = cc.Material.Friction;
			fixtureDef.restitution = cc.Material.Restitution;
			fixtureDef.restitutionThreshold = cc.Material.RestitutionThreshold;
			fixtureDef.density = cc.Material.Density;

			cc.RuntimeFixture = body->CreateFixture(&fixtureDef);
		}
	}

	void PhysicsWorld::BuildWorld()
	{
		// Initialize Box2D world
		m_World = new b2World({ 0.0f, -m_Gravity });
		m_World->SetContactListener((b2ContactListener*)&m_ContactListener);
		
		NetworkManager* networkManager = m_Scene->GetNetworkManager();

		// Create runtime bodies (b2Body)
		for (entt::entity e : m_Scene->m_Registry.view<RigidbodyComponent>())
		{
			Entity entity(e, m_Scene);
			if (networkManager->IsNetModeClient() && entity.HasComponent<NetworkComponent>())
			{
				auto& net = entity.GetComponent<NetworkComponent>();
				if (!net.SimulateOnClient)
					continue;
			}

			CreateRuntimeBody(entity);
		}

		// Add colliders to parent's body
		auto viewBoxes = m_Scene->m_Registry.view<BoxColliderComponent>(entt::exclude<RigidbodyComponent>);
		for (entt::entity e : viewBoxes)
		{
			Entity entity{ e, m_Scene }; 

			// Check AttachToParent property
			auto& collider = entity.GetComponent<BoxColliderComponent>();
			if (!collider.AttachToParent)
				continue;

			// Check if entity has parent
			auto& rc = entity.GetComponent<RelationshipComponent>();
			if (rc.Parent == entt::null)
				continue;

			// Check if parent has RigidbodyComponent
			Entity parent{ rc.Parent, m_Scene };
			if (!parent.HasComponent<RigidbodyComponent>())
				continue;

			auto& rb = parent.GetComponent<RigidbodyComponent>();
			if (rb.RuntimeBody)
				AddFixtureToRuntimeBody(entity, rb.RuntimeBody, true);
		}

		// Add colliders to parent's body
		auto viewCircles = m_Scene->m_Registry.view<CircleColliderComponent>(entt::exclude<RigidbodyComponent, BoxColliderComponent>);
		for (entt::entity e : viewCircles)
		{
			Entity entity{ e, m_Scene };

			// Check AttachToParent property
			auto& collider = entity.GetComponent<CircleColliderComponent>();
			if (!collider.AttachToParent)
				continue;

			// Check if entity has parent
			auto& rc = entity.GetComponent<RelationshipComponent>();
			if (rc.Parent == entt::null)
				continue;

			// Check if parent has RigidbodyComponent
			Entity parent{ rc.Parent, m_Scene };
			if (!parent.HasComponent<RigidbodyComponent>())
				continue;

			auto& rb = parent.GetComponent<RigidbodyComponent>();
			if (rb.RuntimeBody)
				AddFixtureToRuntimeBody(entity, rb.RuntimeBody, true);
		}

		CreateJoints();
	}

	void PhysicsWorld::DestroyWorld()
	{
		if (m_World)
		{
			delete m_World;
			m_World = nullptr;
		}
		m_RuntimeBodies.clear();
		m_FixtureUserData.clear();
	}

	void PhysicsWorld::CreateJoints()
	{
		while (!m_JointsCreateQueue.empty())
		{
			const JointInfo& info = m_JointsCreateQueue.front();

			Entity entityA = m_Scene->FindByID(info.EntityA_UUID);
			Entity entityB = m_Scene->FindByID(info.EntityB_UUID);

			if (!entityA)
			{
				PT_CORE_ERROR("Failed to create joint: Entity A: {} not found!", info.EntityA_UUID);
				m_JointsCreateQueue.pop();
				continue;
			}

			if (!entityB)
			{
				PT_CORE_ERROR("Failed to create joint: Entity B: {} not found!", info.EntityB_UUID);
				m_JointsCreateQueue.pop();
				continue;
			}

			if (!entityA.HasComponent<RigidbodyComponent>())
			{
				PT_CORE_ERROR("Failed to create joint: Entity A: {} ({}) has no RigidbodyComponent", info.EntityA_UUID, entityA.GetTag());
				m_JointsCreateQueue.pop();
				continue;
			}

			if (!entityB.HasComponent<RigidbodyComponent>())
			{
				PT_CORE_ERROR("Failed to create joint: Entity B: {} ({}) has no RigidbodyComponent", info.EntityB_UUID, entityB.GetTag());
				m_JointsCreateQueue.pop();
				continue;
			}

			auto& rbA = entityA.GetComponent<RigidbodyComponent>();
			auto& rbB = entityB.GetComponent<RigidbodyComponent>();
			auto& transform = entityA.GetTransform();

			switch (info.Type)
			{
			case JointType::Revolute:
			{
				b2RevoluteJointDef jointDef;
				b2Vec2 anchorPoint = { transform.WorldPosition.x, transform.WorldPosition.y };
				jointDef.Initialize(rbA.RuntimeBody, rbB.RuntimeBody, anchorPoint);
				jointDef.collideConnected = false;

				b2Joint* joint = m_World->CreateJoint(&jointDef);
				m_Joints.push_back(joint);
				break;
			}
			default:
				PT_CORE_ASSERT("Invalid JointType");
				break;
			}

			m_JointsCreateQueue.pop();
		}
	}

	void PhysicsWorld::ProcessCreatedEntities()
	{
		PROFILE_FUNCTION();

		// Initialize entities created during game runtime
		if (m_EntitiesToInitialize.size())
		{
			auto& vec = m_EntitiesToInitialize;
			vec.erase(std::unique(vec.begin(), vec.end()), vec.end());

			NetworkManager* networkManager = m_Scene->GetNetworkManager();

			for (auto& entity : m_EntitiesToInitialize)
			{
				if (entity.HasComponent<RigidbodyComponent>())
				{
					if (networkManager->IsNetModeClient())
					{
						if (entity.HasComponent<NetworkComponent>())
						{
							auto& net = entity.GetComponent<NetworkComponent>();
							if (!net.SimulateOnClient)
								continue;
						}

						auto& hierarchy = entity.GetComponent<RelationshipComponent>();
						auto& rb = entity.GetComponent<RigidbodyComponent>();
						if (rb.AttachToParent && hierarchy.Parent != entt::null)
						{
							Entity parent(hierarchy.Parent, m_Scene);
							if (parent.HasComponent<NetworkComponent>())
							{
								auto& net = parent.GetComponent<NetworkComponent>();
								if (!net.SimulateOnClient)
									continue;
							}
						}
					}

					m_Scene->CalculateEntityWorldPosition(entity, false);
					CreateRuntimeBody(entity);
					continue;
				}

				if (entity.HasComponent<BoxColliderComponent>())
				{
					auto& collider = entity.GetComponent<BoxColliderComponent>();
					if (!collider.AttachToParent)
						continue;
				}

				if (entity.HasComponent<CircleColliderComponent>())
				{
					auto& collider = entity.GetComponent<CircleColliderComponent>();
					if (!collider.AttachToParent)
						continue;
				}

				auto& rc = entity.GetComponent<RelationshipComponent>();
				if (rc.Parent != entt::null)
				{
					Entity parent{ rc.Parent, m_Scene };

					if (networkManager->IsNetModeClient() && parent.HasComponent<NetworkComponent>())
					{
						auto& net = parent.GetComponent<NetworkComponent>();
						if (!net.SimulateOnClient)
							continue;
					}

					AddFixtureToRuntimeBody(entity, GetRuntimeBody(parent.GetUUID()));
				}
			}
			CreateJoints();
			m_EntitiesToInitialize.clear();
		}
	}

	void PhysicsWorld::Update(float ts)
	{
		PROFILE_FUNCTION();

		// Update physics world
		m_World->Step(ts, m_PhysicsVelocityIterations, m_PhysicsPositionIterations);
		auto view = m_Scene->m_Registry.view<IDComponent, TransformComponent, RigidbodyComponent>();
		for (auto entity : view)
		{
			auto [id, transform, rb] = view.get<IDComponent, TransformComponent, RigidbodyComponent>(entity);
			
			if (!rb.RuntimeBody)
				continue;
			
			b2Body* body = rb.RuntimeBody;
			
			// Retrieve positions of entities
			transform.WorldPosition.x = body->GetPosition().x;
			transform.WorldPosition.y = body->GetPosition().y;
			transform.Rotation = glm::degrees(body->GetAngle());
		}
	}

	PhysicsContactListener::PhysicsContactListener(Scene* scene)
		: m_Scene(scene)
	{
	}

#define CALL_CONTACT_CALLBACK_FUNCTION(callback_func, ...) \
	Entity* entityA = (Entity*)(contact->GetFixtureA()->GetUserData().pointer); \
	Entity* entityB = (Entity*)(contact->GetFixtureB()->GetUserData().pointer); \
	if (!entityA->IsValid() || !entityB->IsValid()) return; \
	PhysicsContactCallback* callbackA = nullptr; \
	PhysicsContactCallback* callbackB = nullptr; \
	if (entityA->HasComponent<BoxColliderComponent>()) \
		callbackA = &entityA->GetComponent<BoxColliderComponent>().ContactCallback; \
	if (entityB->HasComponent<BoxColliderComponent>()) \
		callbackB = &entityB->GetComponent<BoxColliderComponent>().ContactCallback; \
	if (entityA->HasComponent<CircleColliderComponent>()) \
		callbackA = &entityA->GetComponent<CircleColliderComponent>().ContactCallback; \
	if (entityB->HasComponent<CircleColliderComponent>()) \
		callbackB = &entityB->GetComponent<CircleColliderComponent>().ContactCallback; \
	if (callbackA && callbackA->callback_func) \
		callbackA->callback_func(PhysicsContact{ entityB, contact }, __VA_ARGS__); \
	if (callbackB && callbackB->callback_func) \
		callbackB->callback_func(PhysicsContact{ entityA, contact }, __VA_ARGS__); \

	void PhysicsContactListener::BeginContact(b2Contact* contact)
	{
		if (contact->IsEnabled())
		{
			CALL_CONTACT_CALLBACK_FUNCTION(OnBegin);
			if (callbackA)
				callbackA->ContactCount++;
			if (callbackB)
				callbackB->ContactCount++;
		}
	}

	void PhysicsContactListener::EndContact(b2Contact* contact)
	{
		if (contact->IsEnabled())
		{
			CALL_CONTACT_CALLBACK_FUNCTION(OnEnd);
			if (callbackA)
				callbackA->ContactCount--;
			if (callbackB)
				callbackB->ContactCount--;
		}
	}

	void PhysicsContactListener::PreSolve(b2Contact* contact, const b2Manifold* oldManifold)
	{
		CALL_CONTACT_CALLBACK_FUNCTION(OnPreSolve, oldManifold);
	}

	void PhysicsContactListener::PostSolve(b2Contact* contact, const b2ContactImpulse* impulse)
	{
		CALL_CONTACT_CALLBACK_FUNCTION(OnPostSolve, impulse);
	}

}
