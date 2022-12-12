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

		void OnBeginPlay();
		void OnEndPlay();
		
		Entity CreateEntity(const std::string& name = "Unnamed entity");
		Entity CreateEntityWithID(UUID id, const std::string& name = "Unnamed entity");

		void DestroyEntity(Entity entity);
		void DestroyAllEntities();

		Entity FindByID(UUID id);
		Entity FindByTag(const std::string& tag);
		std::vector<Entity> FindAllByTag(const std::string& tag);
		
		b2Body* GetBox2DRigidBody(UUID id);

		void SetPrimaryCamera(Shared<Camera> camera);
		Shared<Camera> GetPrimaryCamera();

		glm::vec2 GetMouseWorldPosition() const;

		uint32_t GetEntitiesCount() const;
		uint32_t GetScriptedEntitiesCount() const;
		
		void OnUpdate(float ts);
	
	private:
		void RenderScene(const Camera& camera);

		glm::mat4 CalculateParentTransform(entt::entity parent);

	private:
		std::string m_SceneName;
		entt::registry m_Registry;
		Shared<Camera> m_PrimaryCamera;
		Camera m_DefaultCamera;

		bool m_PlayState = false;

		b2World* m_World = nullptr;
		std::unordered_map<UUID, b2Body*> m_RigidBodies;

		friend class Entity;
		friend class EditorOverlay;
		friend class SceneSerializer;
	};

}