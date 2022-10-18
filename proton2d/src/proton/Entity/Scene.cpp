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
		Renderer::SetClearColor(s_ClearColor);
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
		entity.AddComponent<RelationshipComponent>();
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
					if (scriptData.ScriptInstance)
					{
						scriptData.ScriptInstance->m_Entity = Entity{ this, entity };
						scriptData.ScriptInstance->OnCreate();
					}
					else
					{
						assert(false && "Script instantiation failed!");
					}
				}

				scriptData.ScriptInstance->OnUpdate(ts);
			}
		});

		if (!m_PrimaryCamera)
			RenderScene(m_DefaultCamera);
		else
			RenderScene(*m_PrimaryCamera);
	}

	void Scene::RenderScene(const Camera& camera)
	{
		Renderer::Clear();
		Renderer::BeginScene(camera);

		auto renderable = m_Registry.group<TransformComponent>(entt::get<SpriteComponent, RelationshipComponent>);

		for (auto entity : renderable)
		{
			auto [transform, sprite, relationship] = renderable.get<TransformComponent, SpriteComponent, RelationshipComponent>(entity);

			glm::mat4 transformMatrix = glm::translate(glm::mat4(1.0f), transform.Position);
			
			if (relationship.Parent != entt::null)
				transformMatrix = CalculateWorldTransform(transformMatrix, relationship.Parent);

			glm::vec3 spriteFlip = { 1.0f, 1.0f, 1.0f };
			if (sprite.Sprite)
			{
				if (sprite.Sprite->m_FlipX)
					spriteFlip.x = -1.0f;

				if (sprite.Sprite->m_FlipY)
					spriteFlip.y = -1.0f;
			}
			
			transformMatrix *= glm::rotate(glm::mat4(1.0f), glm::radians(transform.Rotation), { 0.0f, 0.0f, 1.0f })
				* glm::scale(glm::mat4(1.0f), spriteFlip * glm::vec3{ transform.Scale.x, transform.Scale.y, 1.0f });

			if (sprite.Sprite)
				Renderer::DrawQuad(transformMatrix, sprite.Sprite, sprite.Color);
			else
				Renderer::DrawQuad(transformMatrix, sprite.Color);
		}

		Renderer::EndScene();
	}

	glm::mat4 Scene::CalculateWorldTransform(glm::mat4 localTransform, entt::entity parent)
	{
		auto [parentTransform, parentRelationship] = m_Registry.get<TransformComponent, RelationshipComponent>(parent);

		if (parentRelationship.Parent != entt::null)
			localTransform = CalculateWorldTransform(localTransform, parentRelationship.Parent);

		return localTransform
			* glm::translate(glm::mat4(1.0f), parentTransform.Position)
			* glm::inverse(localTransform)
			* glm::rotate(glm::mat4(1.0f), glm::radians(parentTransform.Rotation), { 0.0f, 0.0f, 1.0f })
			* glm::scale(glm::mat4(1.0f), { parentTransform.Scale.x, parentTransform.Scale.y, 1.0f })
			* localTransform;
	}

	void Scene::SetPrimaryCamera(Entity& cameraEntity)
	{
		SetPrimaryCamera(cameraEntity.GetComponent<CameraComponent>().Camera);
	}

	void Scene::SetPrimaryCamera(Shared<Camera> camera)
	{
		m_PrimaryCamera = camera;
	}

	size_t Scene::GetScriptedEntitiesCount() const
	{
		return m_Registry.view<ScriptComponent>().size();
	}

}