#pragma once

#include "proton/Graphics/Camera.h"
#include "proton/Events/Event.h"
#include "proton/Core/UUID.h"

#include <entt/entt.hpp>

class b2Body;

namespace proton {

	// Forward declaration
	class Entity;
	class PhysicsWorld;

	enum class SceneState
	{
		Stop, Play, Paused
	};

	class Scene
	{
	public:
		Scene(const std::string& name = "Unnamed scene");
		~Scene();
		
		// Create entitiy with random identifier (UUID)
		Entity CreateEntity(const std::string& name = "Entity");

		// Create entitiy with specific identifier (UUID)
		Entity CreateEntityWithID(UUID id, const std::string& name = "Entity");

		// Destroy specific entity
		void DestroyEntity(Entity entity, bool popHierachy = true);

		// Destroy all entities on the scene
		void DestroyAll();

		// Find entity by id (UUID)
		Entity FindByID(UUID id);

		// Find entity by tag from TagComponent
		Entity FindByTag(const std::string& tag);

		// Find all entities that have specific tag
		std::vector<Entity> FindAllByTag(const std::string& tag);

		// TODO: Add GetEntitiesWithComponents

		// Camera methods
		void SetPrimaryCameraEntity(Entity entity);
		Shared<Camera> GetPrimaryCamera();
		Entity GetPrimaryCameraEntity();
		glm::vec3 GetPrimaryCameraPosition();

		// Cursor methods
		glm::vec2 GetCursorPosition();
		std::vector<Entity> GetCursorEntities();
		bool IsCursorOverEntity(Entity entity);

		// Get by UUID the Box2D body from physics world during game runtime. 
		// - Entity must have RigidbodyComponent
		b2Body* GetRuntimeBody(UUID id);

		// TODO: remove
		b2Body* CreateRuntimeBody(Entity entity);

		// Set renderer screen clear color
		void SetScreenClearColor(const glm::vec4& color);

		// Get scene filepath (relative to "scenes" directory)
		const std::string& GetFilepath() const { return m_SceneFilepath; }

		// SceneState::Play, SceneState::Play, SceneState::Stop
		SceneState GetSceneState() const { return m_SceneState; }

		// Set scene state to 'Play', create physics world
		void BeginPlay();

		// Set scene state to 'Pause'
		void Pause(bool pause = true);

		// Set scene state to 'Stop', destroy physics world and script instances
		void Stop();

	private:
		void OnUpdate(float ts);
		void RenderScene(const Camera& camera);
		void OnViewportResize(uint32_t width, uint32_t height);
		void DestroyChildEntities(Entity entity);

		uint32_t GetEntitiesCount() const;
		uint32_t GetScriptedEntitiesCount() const;

	private:
		SceneState m_SceneState = SceneState::Stop;

		// General
		std::string m_SceneName;
		// TODO: Refactor
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
		PhysicsWorld* m_PhysicsWorld = nullptr;

		friend class Application;
		friend class Entity;
		friend class SceneSerializer;
		friend class SceneManager;

		friend class EditorLayer;
		friend class EditorCamera;
		friend class MiscellaneousPanel;
		friend class InspectorPanel;
		friend class SceneHierarchyPanel;
		friend class ScenePanel;

		friend class PhysicsWorld;
	};

}
