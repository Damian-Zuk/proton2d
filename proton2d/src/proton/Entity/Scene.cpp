#include "pch.h"
#include "proton/Entity/Scene.h"
#include "proton/Entity/Entity.h"
#include "proton/Renderer/Renderer.h"
#include "proton/Core/Application.h"

namespace proton {

	Scene::Scene()
		: m_Camera(Application::Get().GetWindow().GetAspectRatio())
	{
	}

	Scene::~Scene()
	{
	}

	Entity Scene::CreateEntity(const std::string& name)
	{
		Entity entity = Entity{ this, m_Registry.create() };
		auto& tag = entity.AddComponent<TagComponent>();
		tag.Tag = name;
		entity.AddComponent<TransformComponent>();
		return entity;
	}

	void Scene::OnUpdate(float ts)
	{
		auto& renderable = m_Registry.group<TransformComponent>(entt::get<SpriteComponent>);
		Renderer::Clear({ 0.1f, 0.12f, 0.16f, 1.0f });
		Renderer::BeginScene(m_Camera);
		for (auto& entity : renderable)
		{
			auto& [transform, sprite] = renderable.get<TransformComponent, SpriteComponent>(entity);
			Renderer::DrawQuad(transform, { 1.0f, 1.0f, 1.0f, 1.0f });
		}
		Renderer::EndScene();
	}

}