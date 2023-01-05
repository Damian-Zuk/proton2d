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
#include <box2d/b2_contact.h>

namespace proton {

	constexpr static int32_t s_PhysicsVelocityIterations = 5;
	constexpr static int32_t s_PhysicsPositionIterations = 5;

	Scene::Scene(const std::string& name)
		: m_SceneName(name), m_DefaultCamera(CreateShared<Camera>()), m_ContactListener(this)
	{
#if PROTON_EDITOR
		m_SceneState = SceneState::EditMode;
#else
		m_SceneState = SceneState::PlayMode;
#endif
	}

	Scene::~Scene()
	{
		OnEndPlay();
		DestroyAll();
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
#endif
	}

	std::string Scene::GetFilepath()
	{
		return m_SceneFilepath;
	}

	b2Body* Scene::CreateBox2DRuntimeBody(Entity entity)
	{
		auto& uuid = entity.GetComponent<IDComponent>().ID;
		auto& transform = entity.GetComponent<TransformComponent>();
		auto& rb = entity.GetComponent<RigidbodyComponent>();
		
		// Create body definition
		b2BodyDef bodyDef;
		bodyDef.type = rb.Type;
		bodyDef.position.Set(transform.Position.x, transform.Position.y);
		bodyDef.angle = glm::radians(transform.Rotation);
		bodyDef.userData.pointer = reinterpret_cast<uintptr_t>(&uuid);

		// Create box2d body
		b2Body* body = m_World->CreateBody(&bodyDef);
		body->SetFixedRotation(rb.FixedRotation);

		if (entity.HasComponent<BoxColliderComponent>())
		{
			auto& bc = entity.GetComponent<BoxColliderComponent>();
			b2FixtureDef fixtureDef; b2PolygonShape shape;

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
			fixtureDef.userData.pointer = reinterpret_cast<uintptr_t>(&uuid);
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
		m_World->SetContactListener((b2ContactListener*)&m_ContactListener);
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
		m_EntityMap[id] = entity;
		return entity;
	}

	void Scene::DestroyEntity(Entity entity)
	{
		if (entity.HasComponent<ScriptComponent>())
		{
			auto& scriptComponent = entity.GetComponent<ScriptComponent>();
			for (auto& kv : scriptComponent.Scripts)
				kv.second->OnDestroy();

			entity.DeleteAllScriptInstances();
		}
			
		m_EntityMap.erase(entity.GetUUID());
		m_Registry.destroy(entity.m_Handle);
	}

	void Scene::DestroyAll()
	{
		m_Registry.each([&](entt::entity e) 
		{
			Entity entity{ this, e };
			DestroyEntity(entity);
		});
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
					for (auto& [scriptName, scriptInstance] : scriptComponent.Scripts)
					{
						if (!scriptInstance->m_Initialized)
						{
							scriptInstance->m_Entity = Entity{ this, entity };
							scriptInstance->OnCreate();
							scriptInstance->m_Initialized = true;
						}
						scriptInstance->OnUpdate(ts);
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
		if (m_EntityMap.find(id) != m_EntityMap.end())
			return m_EntityMap.at(id);
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

		}

		// ***************************************************
		// Render entities with TilemapSpriteComponent
		// ***************************************************
		auto renderableTilemapSprite = m_Registry.group<TilemapSpriteComponent>(entt::get<TransformComponent>);
		for (auto e : renderableTilemapSprite)
		{
			PROFILE_SCOPE("entity_render_tilemap_sprite");
			auto [transform, tsp] = renderableTilemapSprite.get<TransformComponent, TilemapSpriteComponent>(e);
			auto& tilemap = tsp.TilemapSprite;
			auto& spritesheet = tilemap.m_Spritesheet;

			if (spritesheet)
			{
				// Tilemap entity transform matrix
				glm::mat4 transformMatrix = GetTransform(transform.Position, glm::vec2{1.0f}, transform.Rotation);

				uint32_t x = 0, y = 0;
				for (auto& column : tilemap.m_Tilemap)
				{
					for (auto& tile : column)
					{
						if (tile != TILEMAP_BLANK_TILE)
						{
							// Tile local transform matrix
							glm::mat4 tileTransformMatrix = GetTransform({
									(x - tilemap.m_Width / 2.0f + 0.5f) * tilemap.m_TileScale.x,
									(y - tilemap.m_Height / 2.0f + 0.5f) * tilemap.m_TileScale.y, 0
								}, tilemap.m_TileScale);

							Renderer::DrawQuad(transformMatrix * tileTransformMatrix, spritesheet->GetTexture(),
								spritesheet->GetTextureCoords(tile.x, tile.y), tsp.Color);
						}
						y++;
					}
					y = 0; x++;
				}
			}
		}

#if PROTON_EDITOR

		EditorOverlay* editor = EditorOverlay::Get();

		// Draw box colliders
		auto view = m_Registry.view<TransformComponent, BoxColliderComponent>();
		for (auto entity : view)
		{
			auto [transform, bc] = view.get<TransformComponent, BoxColliderComponent>(entity);

			bool drawAll = editor->m_ShowAllColliders;
			// Check if current entity is selected entity and show selection collider is enabled
			bool drawSelected = editor->m_ShowSelectionCollider && EditorOverlay::GetInspectorContext().m_Handle == entity;

			if (drawAll || drawSelected)
			{
				float zPos = (drawAll && drawSelected) ? 0.205f : 0.2f;
				glm::vec4 color = (drawAll && drawSelected) ? glm::vec4{ 0.9f, 0.3f, 0.3f, 0.5f } : glm::vec4{ 0.9f, 0.6f, 0.3f, 0.5f };
				glm::vec3 position = { transform.Position.x + bc.Offset.x, transform.Position.y + bc.Offset.y, zPos };
				glm::vec3 scale = { bc.Size.x * transform.Scale.x, bc.Size.y * transform.Scale.y, 1.0f };
				glm::mat4 transformMatrix = GetTransform(position, scale, transform.Rotation);

				Renderer::DrawQuad(transformMatrix, color);
			}
		}

		// Draw selected entity outline
		Entity selectedEntity = EditorOverlay::GetInspectorContext();

		if (selectedEntity && editor->m_ShowSelectionOutline)
		{
			auto& transform = selectedEntity.GetComponent<TransformComponent>();
			float padding = glm::sqrt(GetPrimaryCamera()->GetZoomLevel()) * 0.05f;
			glm::vec3 position = { transform.Position.x, transform.Position.y, 0.21f };
			glm::vec3 scale = { transform.Scale.x + padding, transform.Scale.y + padding, 1.0f };
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
		const glm::vec2& mousePos = GetMouseWorldPosition();
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
				glm::vec2 rotationCenter = { transform.Position.x, transform.Position.y };
				point -= rotationCenter;
				point = {
					point.x * cosinus - point.y * sinus + rotationCenter.x,
					point.x * sinus + point.y * cosinus + rotationCenter.y
				};
			}

			// Check if point is inside entity bounding box
			const glm::vec3& position = transform.Position;
			const glm::vec2& scale = transform.Scale;
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

	PhysicsContactListener::PhysicsContactListener(Scene* scene)
		: m_Scene(scene)
	{
	}

#define GET_CONTACT_ENTITIES()\
	b2Fixture* fixtureA = contact->GetFixtureA();\
	b2Fixture* fixtureB = contact->GetFixtureB();\
	UUID uuidA = *(UUID*)(fixtureA->GetUserData().pointer);\
	UUID uuidB = *(UUID*)(fixtureB->GetUserData().pointer);\
	Entity entityA = m_Scene->FindByID(uuidA);\
	Entity entityB = m_Scene->FindByID(uuidB);\
	if (!entityA.IsValid() || !entityB.IsValid()) return; \
	auto& bcA = entityA.GetComponent<BoxColliderComponent>();\
	auto& bcB = entityB.GetComponent<BoxColliderComponent>();

	void PhysicsContactListener::BeginContact(b2Contact* contact)
	{
		GET_CONTACT_ENTITIES();
		
		if (bcA.ContactCallback.OnBeginContactFunction)
			bcA.ContactCallback.OnBeginContactFunction({ uuidB, contact });
		
		if (bcB.ContactCallback.OnBeginContactFunction)
			bcB.ContactCallback.OnBeginContactFunction({ uuidA, contact });
	}

	void PhysicsContactListener::EndContact(b2Contact* contact)
	{
		GET_CONTACT_ENTITIES();

		if (bcA.ContactCallback.OnEndContactFunction)
			bcA.ContactCallback.OnEndContactFunction({ uuidB, contact });

		if (bcB.ContactCallback.OnEndContactFunction)
			bcB.ContactCallback.OnEndContactFunction({ uuidA, contact });
	}

	void PhysicsContactListener::PreSolve(b2Contact* contact, const b2Manifold* oldManifold)
	{
		GET_CONTACT_ENTITIES();

		if (bcA.ContactCallback.OnPreSolveFunction)
			bcA.ContactCallback.OnPreSolveFunction({ uuidB, contact }, oldManifold);

		if (bcB.ContactCallback.OnPreSolveFunction)
			bcB.ContactCallback.OnPreSolveFunction({ uuidA, contact }, oldManifold);
	}

	void PhysicsContactListener::PostSolve(b2Contact* contact, const b2ContactImpulse* impulse)
	{
		GET_CONTACT_ENTITIES();

		if (bcA.ContactCallback.OnPostSolveFunction)
			bcA.ContactCallback.OnPostSolveFunction({ uuidB, contact }, impulse);

		if (bcB.ContactCallback.OnPostSolveFunction)
			bcB.ContactCallback.OnPostSolveFunction({ uuidA, contact }, impulse);
	}

}