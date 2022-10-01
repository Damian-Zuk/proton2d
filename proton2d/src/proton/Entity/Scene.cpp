#include "pch.h"
#include "proton/Entity/Scene.h"
#include "proton/Entity/Entity.h"
#include "proton/Graphics/Renderer.h"
#include "proton/Core/Application.h"
#include "proton/Entity/EntityScript.h"

namespace proton {

	constexpr static glm::vec4 s_ClearColor = { 0.1f, 0.12f, 0.16f, 1.0f };

	Scene::Scene(const std::string& name)
		: m_SceneName(name)
	{
	}

	Scene::~Scene()
	{
		m_Registry.view<ScriptComponent>().each([=](auto entity, auto& scriptComponent)
		{
			for (auto& [scriptName, scriptData] : scriptComponent.Scripts)
				scriptData.DestroyInstanceFunction();
		});
	}
	 
	Entity Scene::CreateEntity(const std::string& name)
	{
		Entity entity = Entity{ this, m_Registry.create() };
		entity.AddComponent<TagComponent>().Tag = name;
		entity.AddComponent<TransformComponent>();
		return entity;
	}

	void Scene::DestroyEntity(entt::entity entity)
	{
		m_Registry.destroy(entity);
	}

	void Scene::OnUpdate(float ts)
	{
		m_Registry.view<ScriptComponent>().each([=](auto entity, auto& scriptComponent) 
		{
			for (auto& [scriptName, scriptData] : scriptComponent.Scripts)
			{
				if (!scriptData.ScriptInstance)
				{
					scriptData.CreateInstanceFunction();
					scriptData.ScriptInstance->m_Entity = Entity{ this, entity };
					scriptData.ScriptInstance->OnCreate();
				}

				scriptData.ScriptInstance->OnUpdate(ts);
			}
		});

		if (!m_PrimaryCamera)
			RenderScene(m_DefaultCamera);
		else
			RenderScene(*m_PrimaryCamera);
	}

	void Scene::RenderScene(Camera& camera)
	{
		auto renderable = m_Registry.group<TransformComponent>(entt::get<SpriteComponent>);

		Renderer::Clear(s_ClearColor);
		Renderer::BeginScene(camera);

		for (auto entity : renderable)
		{
			auto [transform, sprite] = renderable.get<TransformComponent, SpriteComponent>(entity);
			if (sprite.Sprite)
				Renderer::DrawQuad(transform, sprite.Sprite, sprite.TilingFactor, sprite.Color);
			else
				Renderer::DrawQuad(transform, sprite.Color);
		}

		Renderer::EndScene();
	}

	void Scene::SetPrimaryCamera(Entity& cameraEntity)
	{
		SetPrimaryCamera(cameraEntity.GetComponent<CameraComponent>().Camera);
	}

	void Scene::SetPrimaryCamera(Shared<Camera> camera)
	{
		m_PrimaryCamera = camera;
	}

}