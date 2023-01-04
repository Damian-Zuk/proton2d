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
		EditMode, PlayMode, Paused
	};

	class Scene
	{
	public:
		Scene(const std::string& name = "Unnamed scene");
		~Scene();

		// Call this function in your AppLayer OnUpdate function to update and render scene.
		void OnUpdate(float ts);
		
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
		std::vector<Entity> GetEntitiesOnMousePosition(); // todo: add default argument "useCollider"

		// Serialize scene to specified file (JSON format)
		void SaveAsFile(const std::string& filepath);

		// Deserialize scene to specified file (JSON format)
		void LoadFromFilepath(const std::string& filepath);

		// Get scene filepath. If scene is unsaved returns empty string
		std::string GetFilepath();
	
	private:
		void OnBeginPlay();
		void OnEndPlay();
		void RenderScene(const Camera& camera);

		uint32_t GetEntitiesCount() const;
		uint32_t GetScriptedEntitiesCount() const;

	private:
		SceneState m_SceneState;
		std::string m_SceneName;
		std::string m_SceneFilepath;

		entt::registry m_Registry;
		std::unordered_map<UUID, Entity> m_EntityMap;

		bool m_EnablePhysics = true;
		float m_WorldGravity = 9.8f;
		b2World* m_World = nullptr;
		std::unordered_map<UUID, b2Body*> m_RuntimeBodies;
		PhysicsContactListener m_ContactListener;

		entt::entity m_PrimaryCameraEntity = entt::null;
		Shared<Camera> m_PrimaryCamera;
		Shared<Camera> m_DefaultCamera;

		friend class Entity;
		friend class EditorOverlay;
		friend class Inspector;
		friend class EditorCamera;
		friend class SceneSerializer;
	};

}