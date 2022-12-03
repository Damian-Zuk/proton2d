#pragma once

#include "proton/Graphics/Camera.h"
#include "proton/Events/Event.h"

#include <entt/entt.hpp>

namespace proton {

	class Entity;

	class Scene
	{
	public:
		Scene(const std::string& name = "Unnamed scene");
		~Scene();

		Entity CreateEntity(const std::string& name = "Unnamed entity");

		void DestroyEntity(entt::entity entity);
		void DestroyAllEntities();

		void OnUpdate(float ts);

		Entity FindByTag(const std::string& tag);
		std::vector<Entity> FindAllByTag(const std::string& tag);
		
		void SetPrimaryCamera(Entity& cameraEntity);
		void SetPrimaryCamera(Shared<Camera> camera);
		Shared<Camera> GetSceneCamera() { return m_PrimaryCamera; }

		glm::vec2 GetMouseWorldPosition();

		size_t GetEntitiesCount() const { return m_Registry.alive(); }
		size_t GetScriptedEntitiesCount() const;
	
	private:
		void RenderScene(const Camera& camera);

		glm::mat4 CalculateParentTransform(entt::entity parent);

	private:
		std::string m_SceneName;
		entt::registry m_Registry;
		glm::vec2 m_MouseWorldPosition;
		Shared<Camera> m_PrimaryCamera;
		Camera m_DefaultCamera;

		friend class Entity;
		friend class EditorOverlay;
		friend class SceneSerializer;
	};

}