#pragma once

#include "proton/Graphics/Camera.h"
#include "proton/Events/Event.h"
#include "proton/Core/UUID.h"
#include "proton/Scene/PhysicsCommon.h"

#include <entt/entt.hpp>

class b2World;
class b2Body;

namespace proton {

	class Entity;

	enum class SceneState
	{
		Stop, Play, Paused
	};

	class Scene
	{
	public:
		Scene(const std::string& name = "Unnamed scene");
		~Scene();

		// Call this function to start playing the scene
		void BeginPlay();

		// Pause the scene
		void Pause(bool pause = true);
		
		// Create entitiy with random identifier (UUID)
		Entity CreateEntity(const std::string& name = "Unnamed entity");

		// Create entitiy with specific identifier (UUID)
		Entity CreateEntityWithID(UUID id, const std::string& name = "Unnamed entity");

		// Destroy specified entity
		void DestroyEntity(Entity entity, bool popHierachy = true);

		// Destroy all entities on the scene
		void DestroyAll();

		// Find entity by id (UUID)
		Entity FindByID(UUID id);

		// Find entity by tag from TagComponent
		Entity FindByTag(const std::string& tag);

		// Find all entities that have specified tag
		std::vector<Entity> FindAllByTag(const std::string& tag);

		// Use this function to retrive Box2D body from game world 
		// during game runtime (SceneState::Play || SceneState::Paused)
		// Entity must have RigidbodyComponent
		b2Body* GetRuntimeBody(UUID id);

		// Camera stuff
		// Enitity is required to have CameraComponent
		void SetPrimaryCameraEntity(Entity entity);
		Shared<Camera> GetPrimaryCamera();
		Entity GetPrimaryCameraEntity();
		glm::vec3 GetPrimaryCameraPosition();

		// Cursor
		glm::vec2 GetCursorWorldPosition();
		bool IsCursorOverEntity(Entity entity);
		std::vector<Entity> GetEntitiesOnCursor();

		// Get scene filepath (relative to "scenes" directory)
		const std::string& GetFilepath() const { return m_SceneFilepath; }

		SceneState GetSceneState() const { return m_SceneState; }
	
		void SetScreenClearColor(const glm::vec4& color);

	private:
		void OnUpdate(float ts);
		void DestroyPhysicsWorld();
		void RenderScene(const Camera& camera);
		void OnViewportResize(uint32_t width, uint32_t height);
		void DestroyChildEntities(Entity entity);

		uint32_t GetEntitiesCount() const;
		uint32_t GetScriptedEntitiesCount() const;

		b2Body* CreateBox2DRuntimeBody(Entity entity);
		void AddFixtureToBox2DBody(b2Body* body, Entity entity);

	private:
		SceneState m_SceneState = SceneState::Stop;

		// General
		std::string m_SceneName;
		std::string m_SceneFilepath = "<Unsaved scene>";
		glm::vec4 m_ClearColor = { 0.1f, 0.12f, 0.16f, 1.0f };

		// ECS
		entt::registry m_Registry;
		std::unordered_map<UUID, Entity> m_EntityMap;

		// Camera stuff
		entt::entity m_PrimaryCameraEntity = entt::null;
		Shared<Camera> m_PrimaryCamera;
		Shared<Camera> m_DefaultCamera;

		// Box2d physics related
		bool m_EnablePhysics = true;
		float m_WorldGravity = 9.8f;
		b2World* m_World = nullptr;
		std::unordered_map<UUID, b2Body*> m_RuntimeBodies;
		std::vector<Unique<UUID>> m_FixturesUserData;

		int m_PhysicsVelocityIterations = 5;
		int m_PhysicsPositionIterations = 5;

		class PhysicsContactListener : public b2ContactListener
		{
		public:
			PhysicsContactListener(Scene* scene);
			virtual void BeginContact(b2Contact* contact) override;
			virtual void EndContact(b2Contact* contact) override;
			virtual void PreSolve(b2Contact* contact, const b2Manifold* oldManifold) override;
			virtual void PostSolve(b2Contact* contact, const b2ContactImpulse* impulse) override;
		private:
			Scene* m_Scene;
		} m_ContactListener;


		friend class Application;
		friend class Entity;
		friend class EditorLayer;
		friend class MiscellaneousPanel;
		friend class InspectorPanel;
		friend class SceneHierarchyPanel;
		friend class ScenePanel;
		friend class EditorCamera;
		friend class SceneSerializer;
		friend class SceneManager;
	};

}