#pragma once

#include "proton/Graphics/Camera.h"
#include "proton/Events/Event.h"
#include "proton/Core/UUID.h"

#include <entt/entt.hpp>

class b2World;
class b2Body;

namespace proton {

	class Entity;

	class Scene
	{
	public:
		Scene(const std::string& name = "Unnamed scene");
		~Scene();

		// Serialize scene to specified file (JSON format)
		void SaveAsFile(const std::string& filepath);

		// Deserialize scene to specified file (JSON format)
		void LoadFromFilepath(const std::string& filepath);
		
		// Create entitiy with random identifier
		Entity CreateEntity(const std::string& name = "Unnamed entity");

		// Create entitiy with specific identifier
		Entity CreateEntityWithID(UUID id, const std::string& name = "Unnamed entity");

		// Remove entitiy from scene
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

		void SetPrimaryCamera(Shared<Camera> camera);
		Shared<Camera> GetPrimaryCamera();

		glm::vec2 GetMouseWorldPosition() const;
		std::vector<Entity> GetEntitiesOnMousePosition(); // todo: add default argument "useCollider"

		// Call this function in your AppLayer update function to update and render scene.
		void OnUpdate(float ts);
	
	private:
		void OnBeginPlay();
		void OnEndPlay();
		void RenderScene(const Camera& camera);

		uint32_t GetEntitiesCount() const;
		uint32_t GetScriptedEntitiesCount() const;

	private:
		std::string m_SceneName;
		entt::registry m_Registry;
		Shared<Camera> m_PrimaryCamera;
		Camera m_DefaultCamera;

		bool m_PlayState = false;
		std::vector<Entity> m_RuntimeEntities;

		bool m_EnablePhysics = true;
		b2World* m_World = nullptr;
		float m_WorldGravity = 9.8f;
		std::unordered_map<UUID, b2Body*> m_RigidBodies;

		friend class Entity;
		friend class EditorOverlay;
		friend class Inspector;
		friend class SceneSerializer;
	};

}