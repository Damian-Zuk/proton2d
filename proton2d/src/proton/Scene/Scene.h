#pragma once

#include "proton/Graphics/Camera.h"
#include "proton/Events/Event.h"
#include "proton/Core/UUID.h"
#include "proton/Scene/Physics.h"

#include <entt/entt.hpp>

class b2World;
class b2Body;

namespace proton {

	class Entity;

	enum class SceneState
	{
		Edit, Play, Paused
	};

	class Scene
	{
	public:
		Scene(const std::string& name = "Unnamed scene");
		Scene(const Scene& other);
		~Scene();

		// Call this function to start playing scene
		void BeginPlay();

		void Pause(bool pause = true);
		
		// Create entitiy with random identifier
		Entity CreateEntity(const std::string& name = "Unnamed entity");

		// Create entitiy with specific identifier
		Entity CreateEntityWithID(UUID id, const std::string& name = "Unnamed entity");

		// Destroy specified entity
		void DestroyEntity(Entity entity);

		// Remove all entities from scene
		void DestroyAll();

		// Find entity by id (UUID)
		Entity FindByID(UUID id);

		// Find entity by tag from TagComponent
		Entity FindByTag(const std::string& tag);

		// Find all entities that have specified tag
		std::vector<Entity> FindAllByTag(const std::string& tag);
		
		// Use this function to create Box2D body in game world during game runtime.
		// Entity must have RigidbodyComponent.
		b2Body* CreateBox2DRuntimeBody(Entity entity);

		// Use this function to retrive Box2D body from game world during game runtime.
		// Entity must have RigidbodyComponent.
		b2Body* GetBox2DRuntimeBody(UUID id);

		// Enitity is required to have CameraComponent
		void SetPrimaryCameraEntity(Entity entity);
		Shared<Camera> GetPrimaryCamera();
		Entity GetPrimaryCameraEntity();
		glm::vec3 GetPrimaryCameraPosition();

		glm::vec2 GetMouseWorldPosition();
		bool IsMouseHoveringEntity(Entity entity);
		std::vector<Entity> GetEntitiesOnMousePosition();

		// Copies entity to destination scene
		Entity CopyEntity(Entity entity, Scene* dstScene);

		// Get scene filepath. If scene is unsaved returns empty string
		const std::string& GetFilepath() const { return m_SceneFilepath; }

		SceneState GetSceneState() const { return m_SceneState; }
	
		void SetScreenClearColor(const glm::vec4& color);

	private:
		void OnUpdate(float ts);
		void EndPlay();
		void RenderScene(const Camera& camera);

		uint32_t GetEntitiesCount() const;
		uint32_t GetScriptedEntitiesCount() const;

	private:
		SceneState m_SceneState = SceneState::Play;
		std::string m_SceneName;
		std::string m_SceneFilepath = "<Unsaved scene>";
		glm::vec4 m_ClearColor = { 0.1f, 0.12f, 0.16f, 1.0f };

		entt::registry m_Registry;
		std::unordered_map<UUID, Entity> m_EntityMap;

		entt::entity m_PrimaryCameraEntity = entt::null;
		Shared<Camera> m_PrimaryCamera;
		Shared<Camera> m_DefaultCamera;

		bool m_EnablePhysics = true;
		float m_WorldGravity = 9.8f;
		b2World* m_World = nullptr;
		std::unordered_map<UUID, b2Body*> m_RuntimeBodies;

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

		bool m_SkipUpdate = false;

		friend class Application;
		friend class Entity;
		friend class EditorOverlay;
		friend class Inspector;
		friend class EditorCamera;
		friend class SceneSerializer;
		friend class SceneManager;
	};

}