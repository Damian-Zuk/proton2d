#pragma once

#include "proton/Entity/Components.h"
#include "proton/Graphics/Camera.h"

#include <entt/entt.hpp>

namespace proton {

	class Entity;

	class Scene
	{
	public:
		Scene(const std::string& name = "Unnamed Scene" );
		~Scene();

		Entity CreateEntity(const std::string& name = "Unnamed Entity");

		void DestroyEntity(entt::entity entity);

		void OnUpdate(float ts);
		
		void SetPrimaryCamera(Entity& cameraEntity);
		void SetPrimaryCamera(Shared<Camera> camera);
		Shared<Camera> GetSceneCamera() { return m_PrimaryCamera; }

		size_t GetEntitiesCount() const { return m_Registry.size(); }
		size_t GetScriptedEntitiesCount() const { return m_Registry.view<ScriptComponent>().size(); }
	
	private:
		void RenderScene(Camera& camera);

	private:
		std::string m_SceneName;
		entt::registry m_Registry;
		Shared<Camera> m_PrimaryCamera;
		Camera m_DefaultCamera;

		friend class Entity;
		friend class EditorOverlay;
	};

}