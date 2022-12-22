#include "pch.h"
#include "proton/Scene/Scene.h"
#include "proton/Scene/Entity.h"
#include "proton/Graphics/Renderer.h"
#include "proton/Core/Application.h"
#include "proton/Scene/EntityScript.h"
#include "proton/Assets/SceneSerializer.h"
#include "proton/Core/Input.h"

#if PROTON_EDITOR
#include "proton/Editor/EditorOverlay.h"
#include <imgui.h>
#endif

#include <box2d/b2_world.h>
#include <box2d/b2_body.h>
#include <box2d/b2_fixture.h>
#include <box2d/b2_polygon_shape.h>


namespace proton {

	constexpr static glm::vec4 s_ClearColor = { 0.1f, 0.12f, 0.16f, 1.0f };
	constexpr static int32_t s_PhysicsVelocityIterations = 5;
	constexpr static int32_t s_PhysicsPositionIterations = 5;

	Scene::Scene(const std::string& name)
		: m_SceneName(name), m_DefaultCamera(CreateShared<Camera>())
	{
		Renderer::SetClearColor(s_ClearColor);
#if PROTON_EDITOR
		EditorOverlay::SetSceneContext(this);
		m_SceneState = SceneState::EditMode;
#endif
	}

	Scene::~Scene()
	{
		m_Registry.view<ScriptComponent>().each([=](auto entity, auto& scriptComponent)
		{
			for (auto& [scriptName, scriptData] : scriptComponent.Scripts)
				scriptData.DestroyInstanceFunction();
		});

		OnEndPlay();
	}

	void Scene::SaveAsFile(const std::string& filepath)
	{
		SceneSerializer::SetContext(this);
		SceneSerializer::Serialize("scenes/" + filepath);
		m_SceneFilepath = filepath;
	}

	void Scene::LoadFromFilepath(const std::string& filepath)
	{
		if (m_SceneState == SceneState::PlayMode)
		{
			OnEndPlay();
#if PROTON_EDITOR
			m_SceneState = SceneState::EditMode;
			ImGui::SetWindowFocus(nullptr);
#else
			OnBeginPlay();
#endif
		}

		DestroyAll();
		SceneSerializer::SetContext(this);
		SceneSerializer::Deserialize("scenes/" + filepath);
		m_SceneFilepath = filepath;

#if PROTON_EDITOR
		EditorOverlay::SetInspectorContext(Entity{});
		if (EditorOverlay::Get()->m_ActiveScene == this)
			EditorOverlay::Get()->m_ActiveSceneFilepath = filepath;
#endif
	}

	std::string Scene::GetFilepath()
	{
		return m_SceneFilepath;
	}

	b2Body* Scene::CreateBox2DRuntimeBody(Entity entity)
	{
		auto& transform = entity.GetComponent<TransformComponent>();
		auto& rb = entity.GetComponent<RigidbodyComponent>();
		
		// Create body definition
		b2BodyDef bodyDef;
		bodyDef.type = rb.Type;
		bodyDef.position.Set(transform.Position.x, transform.Position.y);
		bodyDef.angle = glm::radians(transform.Rotation);

		// Create box2d body
		b2Body* body = m_World->CreateBody(&bodyDef);
		body->SetFixedRotation(rb.FixedRotation);

		if (entity.HasComponent<BoxColliderComponent>())
		{
			auto& bc = entity.GetComponent<BoxColliderComponent>();
			b2FixtureDef fixtureDef; b2PolygonShape shape;

			if (entity.HasComponent<TilemapSpriteComponent>())
			{
				auto& tilemap = entity.GetComponent<TilemapSpriteComponent>().TilemapSprite;
				shape.SetAsBox(tilemap.m_Width * bc.Size.x * transform.Scale.x / 2.0f,
					tilemap.m_Height * bc.Size.y * transform.Scale.y / 2.0f, { bc.Offset.x, bc.Offset.y }, 0);
			}
			else
			shape.SetAsBox(bc.Size.x * transform.Scale.x / 2.0f,
				bc.Size.y * transform.Scale.y / 2.0f, { bc.Offset.x, bc.Offset.y }, 0);

			// Set physics material proporties for body fixture
			const PhysicsMaterial& material = bc.Material;
			fixtureDef.shape = &shape;
			fixtureDef.friction = material.Friction;
			fixtureDef.restitution = material.Restitution;
			fixtureDef.restitutionThreshold = material.RestitutionThreshold;
			fixtureDef.density = material.Density;
			fixtureDef.isSensor = bc.IsSensor;
			body->CreateFixture(&fixtureDef);
		}

		m_RuntimeBodies[entity.GetUUID()] = body;
		return body;
	}

	void Scene::OnBeginPlay()
	{	
		// Initialize physics world
		m_SceneState = SceneState::PlayMode;
		m_World = new b2World({ 0.0f, -m_WorldGravity });
		for (entt::entity entity : m_Registry.view<RigidbodyComponent>())
			CreateBox2DRuntimeBody(Entity{ this, entity });
	}

	void Scene::OnEndPlay()
	{
		if (m_World)
		{
			delete m_World;
			m_World = nullptr;
		}

		m_RuntimeBodies.clear();
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

	void Scene::DestroyAll()
	{
		m_Registry.each([&](auto id) { m_Registry.destroy(id); });
	}

	void Scene::OnUpdate(float ts)
	{
		PROFILE_FUNCTION();

		if (m_SceneState == SceneState::PlayMode)
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
			if (m_EnablePhysics)
			{
				PROFILE_SCOPE("update_physics");
				m_World->Step(ts, s_PhysicsVelocityIterations, s_PhysicsPositionIterations);
				auto view = m_Registry.view<IDComponent, TransformComponent, RigidbodyComponent>();
				for (auto entity : view)
				{
					auto [id, transform] = view.get<IDComponent, TransformComponent>(entity);
					b2Body* body = m_RuntimeBodies.at(id.ID);
					transform.Position.x = body->GetPosition().x;
					transform.Position.y = body->GetPosition().y;
					transform.Rotation = glm::degrees(body->GetAngle());
				}
			}

			// Update animations
			auto view = m_Registry.view<FlipbookAnimationComponent>();
			for (auto entity : view)
				view.get<FlipbookAnimationComponent>(entity).Flipbook->PlayFrame(ts);
		}

		// Render scene
		RenderScene(*GetPrimaryCamera());
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

	b2Body* Scene::GetBox2DRuntimeBody(UUID id)
	{
		return m_RuntimeBodies.at(id);
	}

	static glm::mat4 GetTransform(const glm::vec3& position, const glm::vec2& scale, float rotation = 0.0f)
	{
		return glm::translate(glm::mat4(1.0f), position)
			* glm::rotate(glm::mat4(1.0f), glm::radians(rotation), { 0.0f, 0.0f, 1.0f })
			* glm::scale(glm::mat4(1.0f), { scale.x, scale.y, 1.0f });
	}

	void Scene::RenderScene(const Camera& camera)
	{
		PROFILE_FUNCTION();
		Renderer::Clear();
		Renderer::BeginScene(camera, GetPrimaryCameraPosition());

		// ***************************************************
		// Render entities with SpriteComponent
		// ***************************************************
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

			glm::mat4 transformMatrix = GetTransform(transform.Position, outputScale, transform.Rotation);

			if (sprite.Sprite)
				Renderer::DrawQuad(transformMatrix, sprite.Sprite, sprite.Color);
			else
				Renderer::DrawQuad(transformMatrix, sprite.Color);

#if PROTON_EDITOR
			// Draw box collider
			Entity entity = Entity{ this, e };
			if (entity.HasComponent<BoxColliderComponent>())
			{
				EditorOverlay* editor = EditorOverlay::s_Instance;
				uint8_t drawCollider = editor->m_ShowAllColliders + (EditorOverlay::GetInspectorContext() == entity && editor->m_ShowSelectionCollider);
				if (drawCollider)
				{
					auto& bc = entity.GetComponent<BoxColliderComponent>();
					float z = drawCollider == 1 ? 0.2f : 0.205f;
					glm::vec4 color = drawCollider == 1 ? glm::vec4{ 0.9f, 0.6f, 0.3f, 0.5f } : glm::vec4{ 0.9f, 0.3f, 0.3f, 0.5f };
					glm::vec3 position = { transform.Position.x + bc.Offset.x, transform.Position.y + bc.Offset.y, z };
					glm::vec3 scale = { bc.Size.x * transform.Scale.x, bc.Size.y * transform.Scale.y, 1.0f };
					glm::mat4 transformMatrix = GetTransform(position, scale, transform.Rotation);

					Renderer::DrawQuad(transformMatrix, { 0.9f, 0.6f, 0.3f, 0.5f });
				}
			}
#endif 
		}

		// ***************************************************
		// Render entities with TilemapSpriteComponent
		// ***************************************************
		auto renderableTilemapSprite = m_Registry.group<TilemapSpriteComponent>(entt::get<TransformComponent>);
		for (auto e : renderableTilemapSprite)
		{
			PROFILE_SCOPE("entity_render_tilemap_sprite");
			auto [transform, tilemap] = renderableTilemapSprite.get<TransformComponent, TilemapSpriteComponent>(e);
			auto& spritesheet = tilemap.TilemapSprite.m_Spritesheet;
			if (spritesheet)
			{
				// Tilemap entity transform matrix
				glm::mat4 transformMatrix = GetTransform(transform.Position, glm::vec2{1.0f}, transform.Rotation);

				uint32_t x = 0, y = 0;
				for (auto& column : tilemap.TilemapSprite.m_Tilemap)
				{
					for (auto& tile : column)
					{
						if (tile != TILEMAP_BLANK_TILE)
						{
							// Tile local transform matrix
							glm::mat4 tileTransformMatrix = GetTransform({
									(x - tilemap.TilemapSprite.m_Width / 2.0f + 0.5f) * transform.Scale.x,
									(y - tilemap.TilemapSprite.m_Height / 2.0f + 0.5f) * transform.Scale.y, 0
								}, transform.Scale);

							Renderer::DrawQuad(transformMatrix * tileTransformMatrix, spritesheet->GetTexture(),
								spritesheet->GetTextureCoords(tile.x, tile.y), tilemap.Color);
						}
						y++;
					}
					y = 0; x++;
				}
			}

#if PROTON_EDITOR
			// Draw box collider
			Entity entity = Entity{ this, e };
			if (entity.HasComponent<BoxColliderComponent>())
			{
				EditorOverlay* editor = EditorOverlay::s_Instance;
				uint8_t drawCollider = editor->m_ShowAllColliders + (EditorOverlay::GetInspectorContext() == entity && editor->m_ShowSelectionCollider);
				if (drawCollider)
				{
					auto& bc = entity.GetComponent<BoxColliderComponent>();
					float z = drawCollider == 1 ? 0.2f : 0.205f;
					glm::vec4 color = drawCollider == 1 ? glm::vec4{ 0.9f, 0.6f, 0.3f, 0.5f } : glm::vec4{ 0.9f, 0.3f, 0.3f, 0.5f };

					auto& [width, height] = tilemap.TilemapSprite.GetSize();
					glm::vec3 position = { transform.Position.x + bc.Offset.x, transform.Position.y + bc.Offset.y, z };
					glm::vec2 scale = { width * bc.Size.x * transform.Scale.x, height * bc.Size.y * transform.Scale.y };
					glm::mat4 transformMatrix = GetTransform(position, scale, transform.Rotation);

					Renderer::DrawQuad(transformMatrix, color);
				}
			}
#endif
		}

#if PROTON_EDITOR
		// Draw selected entity outline
		Entity selectedEntity = EditorOverlay::GetInspectorContext();
		EditorOverlay* editor = EditorOverlay::Get();
		if (selectedEntity && editor->m_ShowSelectionOutline)
		{
			auto& transform = selectedEntity.GetComponent<TransformComponent>();
			float padding = glm::sqrt(GetPrimaryCamera()->GetZoomLevel()) * 0.05f;
			glm::vec3 scale;
			
			if (selectedEntity.HasComponent<TilemapSpriteComponent>())
			{
				auto& tilemap = selectedEntity.GetComponent<TilemapSpriteComponent>().TilemapSprite;
				auto& [width, height] = tilemap.GetSize();
				scale = { width * transform.Scale.x + padding, height * transform.Scale.y + padding, 1.0f };
			}
			else
				scale = { transform.Scale.x + padding, transform.Scale.y + padding, 1.0f };

			glm::vec3 position = { transform.Position.x, transform.Position.y, 0.21f };
			glm::mat4 transformMatrix = GetTransform(position, scale, transform.Rotation);
			
			glm::vec4 color = editor->m_ShowSelectionOutline && editor->m_MovingSelection
				            ? glm::vec4{0.8f, 0.8f, 0.2f, 1.0f } : glm::vec4{1.0f};

			Renderer::SetLineWidth(glm::min(50.0f * padding, 1.0f));
			Renderer::DrawRect(transformMatrix, color);
		}
#endif

		Renderer::EndScene();
	}

	void Scene::SetPrimaryCameraEntity(Entity entity)
	{
		if (!entity)
		{
			m_PrimaryCamera = nullptr;
			m_PrimaryCameraEntity = entity.m_Handle;
			return;
		}

		if (entity.HasComponent<CameraComponent>())
		{
			auto& camera = entity.GetComponent<CameraComponent>();
			m_PrimaryCamera = camera.Camera;
			m_PrimaryCameraEntity = entity.m_Handle;
		}
	}

	Shared<Camera> Scene::GetPrimaryCamera()
	{
#if PROTON_EDITOR
		if (m_SceneState != SceneState::EditMode) {
#endif
			return m_PrimaryCamera ? m_PrimaryCamera : m_DefaultCamera;
#if PROTON_EDITOR
		}
		return EditorOverlay::Get()->m_Camera.GetCamera();
#endif
	}

	Entity Scene::GetPrimaryCameraEntity()
	{
		return Entity{ this, m_PrimaryCameraEntity };
	}

	glm::vec2 Scene::GetMouseWorldPosition()
	{
		Window& window = Application::Get().GetWindow();
		uint32_t width = window.GetWidth(), height = window.GetHeight();
		OrthoProjection ortho = GetPrimaryCamera()->GetOrthoProjection();
		glm::vec3 cameraPos = GetPrimaryCameraPosition();
		auto& [mouseX, mouseY] = Input::GetMousePosition();

		return glm::vec2 {
			mouseX / width * ortho.Right * 2.0f + cameraPos.x + ortho.Left,
			mouseY / height * ortho.Bottom * 2.0f + cameraPos.y + ortho.Top
		};
	}

	std::vector<Entity> Scene::GetEntitiesOnMousePosition()
	{
		glm::vec2 mousePos = GetMouseWorldPosition();
		auto view = m_Registry.view<TransformComponent>();
		std::vector<Entity> entities;

		for (auto entity : view)
		{
			auto& transform = view.get<TransformComponent>(entity);

			// Apply entity rotation to point (mouse position)
			// so we can easliy check after if point is inside rectangle
			float sinus = sin(glm::radians(-transform.Rotation));
			float cosinus = cos(glm::radians(-transform.Rotation));
			glm::vec2 point = mousePos;
			if (transform.Rotation)
			{
				glm::vec2 rotationCenter = glm::vec2{ transform.Position.x, transform.Position.y };
				point -= rotationCenter;
				point = glm::vec2{
					point.x * cosinus - point.y * sinus + rotationCenter.x,
					point.x * sinus + point.y * cosinus + rotationCenter.y
				};
			}

			const auto& position = transform.Position;
			glm::vec2 scale;

			// If entity has TilemapSpriteComponent calculate size
			// otherwise use entity TransformComponent scale
			if (m_Registry.any_of<TilemapSpriteComponent>(entity))
			{
				auto& tilemap = m_Registry.get<TilemapSpriteComponent>(entity);
				auto [width, height] = tilemap.TilemapSprite.GetSize();
				scale = { transform.Scale.x * width, transform.Scale.y * height };
			}
			else
				scale = transform.Scale;

			// Check if point is inside entity bounding box
			if (point.x >= position.x - scale.x / 2.0f && point.x <= position.x + scale.x / 2.0f
				&& point.y >= position.y - scale.y / 2.0f && point.y <= position.y + scale.y / 2.0f)
			{
				entities.emplace_back(Entity{ this, entity });
			}
		}
		return entities;
	}

	glm::vec3 Scene::GetPrimaryCameraPosition()
	{
#if PROTON_EDITOR
		if (m_SceneState != SceneState::EditMode) {
#endif
			return m_PrimaryCameraEntity == entt::null ? glm::vec3{ 0.0f }
				: m_Registry.get<TransformComponent>(m_PrimaryCameraEntity).Position;
#if PROTON_EDITOR
		} 
		return EditorOverlay::Get()->m_Camera.GetPosition();
#endif
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