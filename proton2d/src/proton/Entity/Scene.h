#pragma once
#include "proton/Entity/Components.h"
#include "proton/Renderer/Camera.h"

#include <entt/entt.hpp>

namespace proton {

	class Entity;

	class Scene
	{
	public:
		friend class Entity;
		friend class EditorOverlay;

		Scene(const std::string& name = "Unnamed Scene" );
		~Scene();

		Entity CreateEntity(const std::string& name = "Unnamed Entity");

		void OnUpdate(float ts);
		
		void SetPrimaryCamera(Entity& cameraEntity);
		void SetPrimaryCamera(Shared<Camera> camera);
		Shared<Camera> GetSceneCamera() { return m_PrimaryCamera.lock(); }
	
	private:
		void RenderScene(Camera& camera);

	private:
		std::string m_SceneName;
		entt::registry m_Registry;
		std::weak_ptr<Camera> m_PrimaryCamera;
		Camera m_DefaultCamera;
	};

}