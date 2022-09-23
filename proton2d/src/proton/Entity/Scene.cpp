#include "pch.h"
#include "proton/Entity/Scene.h"
#include "proton/Entity/Entity.h"
#include "proton/Graphics/Renderer.h"
#include "proton/Core/Application.h"

namespace proton {

	constexpr static glm::vec4 s_ClearColor = { 0.1f, 0.12f, 0.16f, 1.0f };

	Scene::Scene(const std::string& name)
		: m_SceneName(name)
	{
	}

	Scene::~Scene()
	{
	}

	Entity Scene::CreateEntity(const std::string& name)
	{
		Entity entity = Entity{ this, m_Registry.create() };
		entity.AddComponent<TagComponent>().Tag = name;
		entity.AddComponent<TransformComponent>();
		return entity;
	}

	void Scene::OnUpdate(float ts)
	{
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
			if (sprite.Texture)
				Renderer::DrawQuad(transform, sprite.Texture, sprite.TilingFactor, sprite.Color);
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