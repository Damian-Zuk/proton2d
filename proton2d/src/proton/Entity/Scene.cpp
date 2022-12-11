#include "pch.h"
#include "proton/Entity/Scene.h"
#include "proton/Entity/Entity.h"
#include "proton/Graphics/Renderer.h"
#include "proton/Core/Application.h"
#include "proton/Entity/EntityScript.h"
#include "proton/Core/Input.h"

#ifndef PROTON_DISTRIBUTION
#include "proton/Editor/EditorOverlay.h"
#include <imgui.h>
#endif

namespace proton {

	constexpr static glm::vec4 s_ClearColor = { 0.1f, 0.12f, 0.16f, 1.0f };

	Scene::Scene(const std::string& name)
		: m_SceneName(name)
	{
		Renderer::SetClearColor(s_ClearColor);
		#ifndef PROTON_DISTRIBUTION
			EditorOverlay::SetSceneContext(this);
		#endif
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
		return CreateEntityWithID(UUID(), name);
	}

	Entity Scene::CreateEntityWithID(UUID id, const std::string& name)
	{
		Entity entity = Entity{ this, m_Registry.create() };
		entity.AddComponent<IDComponent>().ID = id;
		entity.AddComponent<TagComponent>().Tag = name;
		entity.AddComponent<TransformComponent>();
		entity.AddComponent<RelationshipComponent>();
		return entity;
	}

	void Scene::DestroyEntity(Entity entity)
	{
		m_Registry.destroy(entity.m_Handle);
	}

	void Scene::DestroyAllEntities()
	{
		m_Registry.each([&](auto id) { m_Registry.destroy(id); });
	}

	void Scene::OnUpdate(float ts)
	{
		PROFILE_FUNCTION();
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

	Entity Scene::FindByID(UUID id)
	{
		auto view = m_Registry.view<IDComponent>();
		for (auto entity : view)
		{
			if (id == view.get<IDComponent>(entity).ID)
				return Entity(this, entity);
		}
		return Entity();
	}

	Entity Scene::FindByTag(const std::string& tag)
	{
		auto view = m_Registry.view<TagComponent>();
		for (auto entity : view)
		{
			if (tag == view.get<TagComponent>(entity).Tag)
				return Entity(this, entity);
		}
		return Entity();
	}

	std::vector<Entity> Scene::FindAllByTag(const std::string& tag)
	{
		auto view = m_Registry.view<TagComponent>();
		std::vector<Entity> entities;
		entities.reserve(view.size());
		for (auto entity : view) 
		{
			if (tag == view.get<TagComponent>(entity).Tag)
				entities.emplace_back(Entity(this, entity));
		}
		return entities;
	}

	void Scene::RenderScene(const Camera& camera)
	{
		PROFILE_FUNCTION();
		Renderer::Clear();
		Renderer::BeginScene(camera);

		// Render entities with SpriteComponent
		auto renderableSprite = m_Registry.group<SpriteComponent>(entt::get<TransformComponent, RelationshipComponent>);
		for (auto e : renderableSprite)
		{
			PROFILE_SCOPE("Entity_Render_Sprite");
			auto [transform, sprite, relationship] = renderableSprite.get<TransformComponent, SpriteComponent, RelationshipComponent>(e);
			
			// Sprite flip
			glm::vec3 outputScale = sprite.Sprite ? glm::vec3{
				transform.Scale.x * (sprite.Sprite->m_FlipX ? -1.0f : 1.0f),
				transform.Scale.y * (sprite.Sprite->m_FlipY ? -1.0f : 1.0f), 1.0f
			} : glm::vec3{ transform.Scale.x, transform.Scale.y, 1.0f };

			// Sprite aspect ratio
			if (sprite.Sprite)
				outputScale.x *= (float)sprite.Sprite->m_Width_px / (float)sprite.Sprite->m_Height_px;

			// Create transform matrix
			glm::mat4 transformMatrix = glm::translate(glm::mat4(1.0f), transform.Position)
				* glm::rotate(glm::mat4(1.0f), glm::radians(-transform.Rotation), { 0.0f, 0.0f, 1.0f })
				* glm::scale(glm::mat4(1.0f), outputScale);

			//if (relationship.Parent != entt::null)
			//	transformMatrix = CalculateParentTransform(relationship.Parent) * transformMatrix;

			if (sprite.Sprite)
				Renderer::DrawQuad(transformMatrix, sprite.Sprite, sprite.Color);
			else
				Renderer::DrawQuad(transformMatrix, sprite.Color);
		}

		// Render entities with TilemapSpriteComponent
		auto renderableTilemapSprite = m_Registry.group<TilemapSpriteComponent>(entt::get<TransformComponent>);
		for (auto e : renderableTilemapSprite)
		{
			PROFILE_SCOPE("Entity_Render_TilemapSprite");
			auto [transform, tilemap] = renderableTilemapSprite.get<TransformComponent, TilemapSpriteComponent>(e);
			auto& spritesheet = tilemap.TilemapSprite.m_Spritesheet;
			if (spritesheet)
			{
				// Tilemap entity transform matrix
				glm::mat4 transformMatrix = glm::translate(glm::mat4(1.0f), transform.Position)
					* glm::rotate(glm::mat4(1.0f), glm::radians(-transform.Rotation), { 0.0f, 0.0f, 1.0f });

				uint32_t x = 0, y = 0;
				for (auto& column : tilemap.TilemapSprite.m_Tilemap)
				{
					for (auto& tile : column)
					{
						if (tile != TILEMAP_BLANK_TILE)
						{
							// Tile local transform matrix
							glm::mat4 tileTransformMatrix = glm::translate(glm::mat4(1.0f), {
									(x - tilemap.TilemapSprite.m_Width / 2.0f + 0.5f) * transform.Scale.x,
									(y - tilemap.TilemapSprite.m_Height / 2.0f + 0.5f) * transform.Scale.y, 0
								}) * glm::scale(glm::mat4(1.0f), glm::vec3{ transform.Scale.x, transform.Scale.y, 1.0f });

							Renderer::DrawQuad(transformMatrix * tileTransformMatrix, spritesheet->GetTexture(),
								spritesheet->GetTextureCoords(tile.x, tile.y), tilemap.Color);
						}
						y++;
					}
					y = 0; x++;
				}
			}
		}

#ifndef PROTON_DISTRIBUTION
		// Draw selected entity outline
		Entity selectedEntity = EditorOverlay::GetInspectorContext();
		if (selectedEntity && EditorOverlay::s_Instance->m_DrawSelectedEntityOutline)
		{
			auto& transform = selectedEntity.GetComponent<TransformComponent>();
			if (selectedEntity.HasComponent<SpriteComponent>())
			{
				glm::mat4 transformMatrix = glm::translate(glm::mat4(1.0f), { transform.Position.x, transform.Position.y, 0.2f })
					* glm::rotate(glm::mat4(1.0f), glm::radians(-transform.Rotation), { 0.0f, 0.0f, 1.0f })
					* glm::scale(glm::mat4(1.0f), glm::vec3{ transform.Scale.x + 0.05f, transform.Scale.y + 0.05f, 1.0f });

				Renderer::SetLineWidth(0.1f);
				Renderer::DrawRect(transformMatrix, { 255,255,255,255 });
			}
			if (selectedEntity.HasComponent<TilemapSpriteComponent>())
			{
				auto& tilemap = selectedEntity.GetComponent<TilemapSpriteComponent>().TilemapSprite;
				auto& [width, height] = tilemap.GetSize();

				glm::mat4 transformMatrix = glm::translate(glm::mat4(1.0f), { transform.Position.x, transform.Position.y, 0.2f })
					* glm::rotate(glm::mat4(1.0f), glm::radians(-transform.Rotation), { 0.0f, 0.0f, 1.0f })
					* glm::scale(glm::mat4(1.0f), glm::vec3{ transform.Scale.x * width + 0.05f, transform.Scale.y * height + 0.05f, 1.0f });

				Renderer::SetLineWidth(0.1f);
				Renderer::DrawRect(transformMatrix, { 255,255,255,255 });
			}
		}
#endif

		Renderer::EndScene();
	}

	glm::mat4 Scene::CalculateParentTransform(entt::entity parent)
	{
		PROFILE_FUNCTION();
		auto [parentTransform, parentRelationship] = m_Registry.get<TransformComponent, RelationshipComponent>(parent);

		glm::mat4 transformMatrix = glm::translate(glm::mat4(1.0f), parentTransform.Position)
			* glm::rotate(glm::mat4(1.0f), glm::radians(parentTransform.Rotation), { 0.0f, 0.0f, 1.0f })
			* glm::scale(glm::mat4(1.0f), { parentTransform.Scale.x, parentTransform.Scale.y, 1.0f });

		if (parentRelationship.Parent != entt::null)
			return CalculateParentTransform(parentRelationship.Parent) * transformMatrix;

		return transformMatrix;
	}

	void Scene::SetPrimaryCamera(Shared<Camera> camera)
	{
		m_PrimaryCamera = camera;
	}

	Shared<Camera> Scene::GetPrimaryCamera()
	{
		return m_PrimaryCamera;
	}

	glm::vec2 Scene::GetMouseWorldPosition() const
	{
		Window& window = Application::Get().GetWindow();
		uint32_t width = window.GetWidth(), height = window.GetHeight();
		OrthoProjection ortho = m_PrimaryCamera->GetOrthoProjection();
		glm::vec3 cameraPos = m_PrimaryCamera->GetPosition();
		auto& [mouseX, mouseY] = Input::GetMousePosition();

		return glm::vec2 {
			mouseX / width * ortho.Right * 2.0f + cameraPos.x + ortho.Left,
			mouseY / height * ortho.Bottom * 2.0f + cameraPos.y + ortho.Top
		};
}

	uint32_t Scene::GetEntitiesCount() const
	{
		return (int32_t)m_Registry.alive();
	}

	uint32_t Scene::GetScriptedEntitiesCount() const
	{
		return (int32_t)m_Registry.view<ScriptComponent>().size();
	}

}