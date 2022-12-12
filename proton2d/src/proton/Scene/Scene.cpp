#include "pch.h"
#include "proton/Scene/Scene.h"
#include "proton/Scene/Entity.h"
#include "proton/Graphics/Renderer.h"
#include "proton/Core/Application.h"
#include "proton/Scene/EntityScript.h"
#include "proton/Core/Input.h"

#ifndef PROTON_DISTRIBUTION
#include "proton/Editor/EditorOverlay.h"
#include <imgui.h>
#endif

#include <box2d/b2_world.h>
#include <box2d/b2_body.h>
#include <box2d/b2_fixture.h>
#include <box2d/b2_polygon_shape.h>


namespace proton {

	constexpr static glm::vec4 s_ClearColor = { 0.1f, 0.12f, 0.16f, 1.0f };
	constexpr int32_t s_PhysicsVelocityIterations = 6;
	constexpr int32_t s_PhysicsPositionIterations = 2;

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

		if (m_PlayState)
			OnEndPlay();
	}

	void Scene::OnBeginPlay()
	{
		// Initialize physics world
		m_World = new b2World({ 0.0f, -9.8f });
		auto view = m_Registry.view<IDComponent, TransformComponent, RigidBodyComponent>();
		for (auto entity : view)
		{
			auto [id, transform, rb] = view.get<IDComponent, TransformComponent, RigidBodyComponent>(entity);

			b2BodyDef bodyDef;
			if (rb.Type == RigidBodyComponent::BodyType::Static)
				bodyDef.type = b2_staticBody;
			else if (rb.Type == RigidBodyComponent::BodyType::Kinematic)
				bodyDef.type = b2_kinematicBody;
			else if (rb.Type == RigidBodyComponent::BodyType::Dynamic)
				bodyDef.type = b2_dynamicBody;

			bodyDef.position.Set(transform.Position.x, transform.Position.y);
			bodyDef.angle = glm::radians(transform.Rotation);

			// Create box2d rigid body
			b2Body* body = m_World->CreateBody(&bodyDef);
			body->SetFixedRotation(rb.FixedRotation);

			if (m_Registry.any_of<BoxColliderComponent>(entity))
			{
				auto& bc = m_Registry.get<BoxColliderComponent>(entity);
				b2PolygonShape shape;
				b2FixtureDef fixtureDef;

				if (m_Registry.any_of<TilemapSpriteComponent>(entity))
				{
					auto& tilemap = m_Registry.get<TilemapSpriteComponent>(entity);
					shape.SetAsBox(tilemap.TilemapSprite.m_Width * bc.Size.x * transform.Scale.x,
						tilemap.TilemapSprite.m_Height * bc.Size.y * transform.Scale.y);
				}
				else
					shape.SetAsBox(bc.Size.x * transform.Scale.x, bc.Size.y * transform.Scale.y);

				fixtureDef.shape = &shape;
				fixtureDef.friction = bc.Friction;
				fixtureDef.restitution = bc.Restitution;
				fixtureDef.restitutionThreshold = bc.RestitutionThreshold;
				fixtureDef.density = bc.Density;
				fixtureDef.isSensor = bc.IsSensor;
				body->CreateFixture(&fixtureDef);
			}

			// Add rigid body to unordered map
			m_RigidBodies[id.ID] = body;
		}
	}

	void Scene::OnEndPlay()
	{
		if (!m_World)
		{
			delete m_World;
			m_World = nullptr;
		}
		m_RigidBodies.clear();
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

		if (m_PlayState)
		{
			// Update scripts
			{
				PROFILE_SCOPE("update_scripts");
				m_Registry.view<ScriptComponent>().each([=](auto entity, auto& scriptComponent)
				{
					for (auto& [scriptName, scriptData] : scriptComponent.Scripts)
					{
						if (!scriptData.Initialized && scriptData.ScriptInstance)
						{
							scriptData.ScriptInstance->m_Entity = Entity{ this, entity };
							scriptData.ScriptInstance->OnCreate();
							scriptData.Initialized = true;
						}
						scriptData.ScriptInstance->OnUpdate(ts);
					}
				});
			}

			// Update physics
			{
				PROFILE_SCOPE("update_physics");
				m_World->Step(ts, s_PhysicsVelocityIterations, s_PhysicsPositionIterations);
				auto view = m_Registry.view<IDComponent, TransformComponent, RigidBodyComponent>();
				for (auto entity : view)
				{
					auto [id, transform, rb] = view.get<IDComponent, TransformComponent, RigidBodyComponent>(entity);
					
					b2Body* body = m_RigidBodies.at(id.ID);
					const auto& position = body->GetPosition();
					transform.Position.x = position.x;
					transform.Position.y = position.y;
					transform.Rotation = glm::degrees(body->GetAngle());
				}
			}
		}

		// Render scene
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

	b2Body* Scene::GetBox2DRigidBody(UUID id)
	{
		return m_RigidBodies.at(id);
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
			PROFILE_SCOPE("entity_render_sprite");
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
				* glm::rotate(glm::mat4(1.0f), glm::radians(transform.Rotation), { 0.0f, 0.0f, 1.0f })
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
			PROFILE_SCOPE("entity_render_tilemap_sprite");
			auto [transform, tilemap] = renderableTilemapSprite.get<TransformComponent, TilemapSpriteComponent>(e);
			auto& spritesheet = tilemap.TilemapSprite.m_Spritesheet;
			if (spritesheet)
			{
				// Tilemap entity transform matrix
				glm::mat4 transformMatrix = glm::translate(glm::mat4(1.0f), transform.Position)
					* glm::rotate(glm::mat4(1.0f), glm::radians(transform.Rotation), { 0.0f, 0.0f, 1.0f });

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
			float zoomLevel = m_PrimaryCamera->GetZoomLevel();
			glm::vec3 padding = { 0.05f * zoomLevel, 0.05f * zoomLevel, 0.0f };
			
			if (selectedEntity.HasComponent<SpriteComponent>())
			{
				glm::mat4 transformMatrix = glm::translate(glm::mat4(1.0f), { transform.Position.x, transform.Position.y, 0.2f })
					* glm::rotate(glm::mat4(1.0f), glm::radians(transform.Rotation), { 0.0f, 0.0f, 1.0f })
					* glm::scale(glm::mat4(1.0f), glm::vec3{ transform.Scale.x, transform.Scale.y , 1.0f } + padding);

				Renderer::SetLineWidth(0.05f * zoomLevel);
				Renderer::DrawRect(transformMatrix, { 255,255,255,255 });
			}
			if (selectedEntity.HasComponent<TilemapSpriteComponent>())
			{
				auto& tilemap = selectedEntity.GetComponent<TilemapSpriteComponent>().TilemapSprite;
				auto& [width, height] = tilemap.GetSize();

				glm::mat4 transformMatrix = glm::translate(glm::mat4(1.0f), { transform.Position.x, transform.Position.y, 0.2f })
					* glm::rotate(glm::mat4(1.0f), glm::radians(transform.Rotation), { 0.0f, 0.0f, 1.0f })
					* glm::scale(glm::mat4(1.0f), glm::vec3{ transform.Scale.x * width , transform.Scale.y * height, 1.0f } + padding);

				Renderer::SetLineWidth(0.05f * zoomLevel);
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