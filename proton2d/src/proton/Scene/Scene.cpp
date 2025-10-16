#include "ptpch.h"
#include "Proton/Scene/Scene.h"
#include "Proton/Scene/Entity.h"
#include "Proton/Graphics/Renderer/Renderer.h"
#include "Proton/Scripting/EntityScript.h"
#include "Proton/Scripting/GameModeBase.h"
#include "Proton/Core/Application.h"
#include "Proton/Core/GameInstance.h"
#include "Proton/Core/Input.h"
#include "Proton/Scene/SceneSerializer.h"
#include "Proton/Scene/PrefabManager.h"
#include "Proton/Utils/Utils.h"
#include "Proton/Physics/PhysicsWorld.h"

#include "Proton/Network/NetworkManager.h"
#include "Proton/Network/NetTransformSystem.h"
#include "Proton/Network/Server.h"
#include "Proton/Network/Client.h"

#ifdef PT_EDITOR
#include "Proton/Editor/EditorLayer.h"
#include "Proton/Editor/Panels/SceneViewportPanel.h"
#include "Proton/Editor/Tools/EditorCamera.h"

#include <imgui.h>
#endif

namespace proton {

	constexpr glm::vec4 DEFAULT_SCENE_SCREEN_CLEAR_COLOR = { 0.24f, 0.37f, 0.67f, 1.0f };

	Scene::Scene(const std::string& filepath, const std::string& gameModeClass)
		:  m_Filepath(filepath), m_ClearColor(DEFAULT_SCENE_SCREEN_CLEAR_COLOR)
	{
		if (gameModeClass.size() && gameModeClass != "GameModeBase")
		{
			m_GameModeClassName = gameModeClass;
			ScriptFactory::Get().InstantiateGameMode(this, m_GameModeClassName);
		}
		else
		{
			m_GameMode = new GameModeBase();
		}

		m_PhysicsWorld = MakeUnique<PhysicsWorld>(this);
	}

	Scene::~Scene()
	{
		if (m_PhysicsWorld->IsInitialized())
			m_PhysicsWorld->DestroyWorld();

		if (m_GameMode)
			delete m_GameMode;

		auto view = m_Registry.view<ScriptComponent>();
		for (auto entity : view)
		{
			auto& component = view.get<ScriptComponent>(entity);
			for (auto& script : component.Scripts)
			{
				delete script.second;
			}
		}
	}

	using ComponentsToCopy =
		ComponentGroup<TransformComponent, PrefabComponent, CameraComponent,
		SpriteComponent, CircleRendererComponent, ResizableSpriteComponent, TextComponent,
		RigidbodyComponent, BoxColliderComponent, CircleColliderComponent,
		NetworkComponent>;

	template<typename... TComponent>
	static void CopyComponent(entt::registry& dst, entt::registry& src, const std::unordered_map<UUID, entt::entity>& enttMap)
	{
		([&]()
		{
			auto view = src.view<TComponent>();
			for (auto srcEntity : view)
			{
				entt::entity dstEntity = enttMap.at(src.get<IDComponent>(srcEntity).ID);
				auto& srcComponent = src.get<TComponent>(srcEntity);
				dst.emplace_or_replace<TComponent>(dstEntity, srcComponent);
			}
		}(), ...);
	}

	template<typename... TComponent>
	static void CopyComponent(ComponentGroup<TComponent...>, entt::registry& dst, entt::registry& src, const std::unordered_map<UUID, entt::entity>& enttMap)
	{
		CopyComponent<TComponent...>(dst, src, enttMap);
	}

	template<typename... TComponent>
	static void CopyComponentIfExists(Entity dst, Entity src)
	{
		([&]()
		{
			if (src.HasComponent<TComponent>())
				dst.AddOrReplaceComponent<TComponent>(src.GetComponent<TComponent>());
		}(), ...);
	}

	template<typename... TComponent>
	static void CopyComponentIfExists(ComponentGroup<TComponent...>, Entity dst, Entity src)
	{
		CopyComponentIfExists<TComponent...>(dst, src);
	}
	// ---------------------------------------------------------------------------------

	Shared<Scene> Scene::CreateSceneCopy(GameInstance* gameInstance)
	{
		Shared<Scene> newScene = MakeShared<Scene>(m_Filepath, m_GameModeClassName);
		newScene->m_ClearColor = m_ClearColor;
		newScene->m_EnablePhysics = m_EnablePhysics;
		newScene->SetPhysicsTickrate(m_PhysicsTickrate);

		newScene->m_EnableNetworking = m_EnableNetworking;

		if (gameInstance)
			newScene->m_GameInstance = gameInstance;
		else	
			newScene->m_GameInstance = m_GameInstance;

		auto& dstSceneRegistry = newScene->m_Registry;
		std::unordered_map<UUID, entt::entity> enttMap;

		// Create entities in new scene
		for (entt::entity srcEntity : m_Registry.view<IDComponent>())
		{
			const auto& id = m_Registry.get<IDComponent>(srcEntity);
			const auto& name = m_Registry.get<TagComponent>(srcEntity).Tag;
			Entity newEntity = newScene->CreateEntityWithUUID(id.ID, name, false);
			newEntity.GetComponent<IDComponent>().RefID = id.RefID;
			enttMap[id.ID] = (entt::entity)newEntity;
		}

		// Set RelationshipComponent in new registry
		for (entt::entity srcEntity : m_Registry.view<RelationshipComponent>())
		{
			entt::entity dstEntity = enttMap.at(m_Registry.get<IDComponent>(srcEntity).ID);
			auto& srcComponent = m_Registry.get<RelationshipComponent>(srcEntity);
			auto& dstComponent = dstSceneRegistry.get<RelationshipComponent>(dstEntity);
			dstComponent.ChildrenCount = srcComponent.ChildrenCount;

			// Set handles to destination registry entities
			if (srcComponent.First != entt::null)
				dstComponent.First = enttMap.at(m_Registry.get<IDComponent>(srcComponent.First).ID);
			
			if (srcComponent.Prev != entt::null)
				dstComponent.Prev = enttMap.at(m_Registry.get<IDComponent>(srcComponent.Prev).ID);
			
			if (srcComponent.Next != entt::null)
				dstComponent.Next = enttMap.at(m_Registry.get<IDComponent>(srcComponent.Next).ID);
			
			if (srcComponent.Parent != entt::null)
				dstComponent.Parent = enttMap.at(m_Registry.get<IDComponent>(srcComponent.Parent).ID);
		}

		// Update new scene root
		for (Entity entity : m_Root)
		{
			newScene->m_Root.push_back(Entity(enttMap.at(entity.GetUUID()), newScene.get()));
		}

		// Copy all components except:
		// IDComponent, TagComponent, RelationshipComponent, ScriptComponent, SpriteAnimationComponent
		CopyComponent(ComponentsToCopy{}, dstSceneRegistry, m_Registry, enttMap);

		// Clear Script Replication Info in NetworkComponent
		auto& netView = dstSceneRegistry.view<NetworkComponent>();
		for (entt::entity entity : netView)
		{
			auto& net = netView.get<NetworkComponent>(entity);
			net.ReplicatedScripts.clear();
		}

		// Create EntityScript instances
		for (entt::entity srcEntity : m_Registry.view<ScriptComponent>())
		{
			entt::entity dstEntity = enttMap.at(m_Registry.get<IDComponent>(srcEntity).ID);
			dstSceneRegistry.emplace<ScriptComponent>(dstEntity);
			
			// Create script copy
			for (auto& it : m_Registry.get<ScriptComponent>(srcEntity).Scripts)
			{
				EntityScript* script = ScriptFactory::Get().AddScriptToEntity(Entity(dstEntity, newScene.get()), it.first);
				// Copy field values
				for (auto& [fieldName, fieldData] : it.second->m_ScriptFields)
				{
					script->SetFieldValueData(fieldName, fieldData.ValuePtr);
				}
			}
		}

		// Set primary camera for new scene
		if (m_PrimaryCameraEntity != entt::null)
		{
			Entity dstPrimaryCameraEntity = newScene->FindByID(Entity(m_PrimaryCameraEntity, this).GetUUID());
			newScene->SetPrimaryCameraEntity(dstPrimaryCameraEntity);
		}

		return newScene;
	}

	void Scene::BeginPlay()
	{	
		if (m_State == SceneState::Play || m_State == SceneState::Paused)
			return; 

	#ifdef PT_EDITOR
		EditorLayer::Get()->OnBeginSceneSimulation(this);
	#endif

		m_State = SceneState::Play;
		m_GameInstance->OnSceneSimulationStart(this);
		
		CalculateWorldPositions();

		if (m_EnablePhysics)
			BuildPhysicsWorld();

		if (m_GameMode)
			m_GameMode->OnCreate();

		m_PhysicsTimeAccumulator = 0.0f;
	}


	void Scene::BuildPhysicsWorld()
	{
		if (!m_PhysicsWorld->IsInitialized())
			m_PhysicsWorld->BuildWorld();
	}

	void Scene::Pause(bool pause)
	{
		if (m_State == SceneState::Stop)
			return;

		m_State = pause ? SceneState::Paused : SceneState::Play;
	}

	void Scene::Stop()
	{
		if (m_State == SceneState::Stop)
			return;

		if (m_PhysicsWorld->IsInitialized())
			m_PhysicsWorld->DestroyWorld();

		m_GameMode->OnDestroy();
		m_GameMode = ScriptFactory::Get().InstantiateGameMode(this, m_GameModeClassName);

		m_State = SceneState::Stop;
		m_GameInstance->OnSceneSimulationStop(this);

	#ifdef PT_EDITOR
		EditorLayer::Get()->OnStopSceneSimulation(this);
	#endif
	}

	Entity Scene::CreateEntity(const std::string& name)
	{
		return CreateEntityWithUUID(UUID(), name);
	}

	Entity Scene::CreateEntityWithUUID(UUID id, const std::string& name, bool addToSceneRoot)
	{
		Entity entity = Entity{ m_Registry.create(), this };
		entity.AddComponent<IDComponent>().ID = id;
		entity.AddComponent<TagComponent>().Tag = name;
		entity.AddComponent<TransformComponent>();
		entity.AddComponent<RelationshipComponent>();
		m_EntityMap[id] = entity;
		
		if (addToSceneRoot)
			m_Root.push_back(entity);

		NetworkManager* networkManager = m_GameInstance->GetNetworkManager();
		if (networkManager->IsNetworkActive() && networkManager->IsNetModeServer())
		{
			networkManager->GetServer()->OnEntitySpawned(this, id);
		}

		return entity;
	}

	Entity Scene::DuplicateEntity(Entity entity, Entity attachTo)
	{
		Entity newEntity = CreateEntity(entity.GetTag());
		newEntity.GetComponent<IDComponent>().RefID = entity.GetComponent<IDComponent>().RefID;

		auto& srcRelation = entity.GetComponent<RelationshipComponent>();
		auto& dstRelation = newEntity.GetComponent<RelationshipComponent>();

		if (attachTo)
		{
			attachTo.AddChildEntity(newEntity);
		}
		else if (srcRelation.Parent != entt::null)
		{
			Entity parent(srcRelation.Parent, this);
			parent.AddChildEntity(newEntity);
		}

		// Copy components ...
		CopyComponentIfExists(ComponentsToCopy{}, newEntity, entity);

		if (entity.HasComponent<ScriptComponent>())
		{
			auto& srcScript = entity.GetComponent<ScriptComponent>();
			auto& dstScript = newEntity.AddComponent<ScriptComponent>();

			for (auto& it : srcScript.Scripts)
			{
				EntityScript* script = ScriptFactory::Get().AddScriptToEntity(newEntity, it.first);
				for (auto& [fieldName, fieldData] : it.second->m_ScriptFields)
				{
					script->SetFieldValueData(fieldName, fieldData.ValuePtr);
				}
			}
		}

		if (srcRelation.ChildrenCount > 0)
		{
			Entity current = Entity(srcRelation.First, this);
			while (current)
			{
				auto& r = current.GetComponent<RelationshipComponent>();
				DuplicateEntity(current, newEntity);
				current = Entity(r.Next, this);
			}
		}

		return newEntity;
	}

	void Scene::DestroyEntity(Entity entity, bool popHierarchy)
	{
		if (!entity.IsValid()) return;

	#ifdef PT_EDITOR
		auto viewport = EditorLayer::GetSceneViewportPanel(this);
		if (entity == viewport->GetSelectedEntity())
			viewport->SetSelectedEntity(Entity{});
	#endif

		// If server is running and entity has NetworkComponent
		// inform clients that entity has been destroyed.
		NetworkManager* networkManager = m_GameInstance->GetNetworkManager();
		if (networkManager->IsNetworkActive() && networkManager->IsNetModeServer()
			&& entity.HasComponent<NetworkComponent>())
		{
			networkManager->GetServer()->OnEntityDespawned(this, entity.GetUUID());
		}

		if (entity.HasComponent<ScriptComponent>())
			entity.DestroyAllScripts();

		if (entity.HasComponent<RigidbodyComponent>())
		{
			if (m_EnablePhysics && m_PhysicsWorld->IsInitialized())
				m_PhysicsWorld->DestroyRuntimeBody(entity.GetUUID());	
		}

		if (m_PrimaryCameraEntity == entity)
		{
			m_PrimaryCameraEntity = entt::null;
			m_PrimaryCamera = nullptr;
		}

		DestroyChildEntities(entity);

		// Update parent hierarchy only for entity which is being deleted
		auto& rc = entity.GetComponent<RelationshipComponent>();
		if (popHierarchy && rc.Parent != entt::null)
		{
			Entity parent{ rc.Parent, this };
			Entity prev{ rc.Prev, this };
			Entity next{ rc.Next, this };

			auto& prc = parent.GetComponent<RelationshipComponent>();
			prc.ChildrenCount--;
			if (prc.First == entity.m_Handle)
				prc.First = rc.Next;

			if (prev)
				prev.GetComponent<RelationshipComponent>().Next = rc.Next;
		
			if (next)
				next.GetComponent<RelationshipComponent>().Prev = rc.Prev;
		}

		if (rc.Parent == entt::null) 
		{
			auto it = std::find(m_Root.begin(), m_Root.end(), entity);
			if (it != m_Root.end())
				m_Root.erase(it);
		}

		m_EntityMap.erase(entity.GetUUID());
		m_Registry.destroy(entity.m_Handle);
	}

	void Scene::DestroyChildEntities(Entity entity)
	{
		auto& rc = entity.GetComponent<RelationshipComponent>();
		Entity current{ rc.First, this };
		while (current)
		{
			entt::entity next = current.GetComponent<RelationshipComponent>().Next;
			DestroyEntity(current, false);
			current = Entity{ next, this };
		}
		rc.ChildrenCount = 0;
		rc.First = entt::null;
	}

	void Scene::DestroyAll()
	{
		m_Registry.view<IDComponent>().each([&](entt::entity e, auto& component)
		{
			Entity entity{ e, this };
			DestroyEntity(entity);
		});
	}

	void Scene::SetEntityLocalPosition(Entity entity, const glm::vec3& position, bool recalculateChildren)
	{
		auto [transform, rc] = m_Registry.get<TransformComponent, RelationshipComponent>(entity.m_Handle);
		transform.WorldPosition = position;
		transform.LocalPosition = position;
		Entity current{ rc.Parent, this };
		while (current)
		{
			auto& crc = current.GetComponent<RelationshipComponent>();
			transform.WorldPosition += current.GetTransform().LocalPosition;
			current = Entity{ crc.Parent, this };
		}

		if (!recalculateChildren)
			return;

		Entity child{ rc.First, this };
		while (child)
		{
			auto& r = child.GetComponent<RelationshipComponent>();
			CalculateEntityWorldPosition(child, true);
			child = Entity(r.Next, this);
		}
	}

	void Scene::SetEntityWorldPosition(Entity entity, const glm::vec3& position, bool recalculateChildren)
	{
		auto [transform, rc] = m_Registry.get<TransformComponent, RelationshipComponent>(entity.m_Handle);
		transform.WorldPosition = position;
		transform.LocalPosition = position;
		
		Entity current{ rc.Parent, this };
		while (current)
		{
			auto& crc = current.GetComponent<RelationshipComponent>();
			transform.LocalPosition -= current.GetTransform().LocalPosition;
			current = Entity{ crc.Parent, this };
		}

		if (!recalculateChildren)
			return;

		Entity child{ rc.First, this };
		while (child)
		{
			auto& r = child.GetComponent<RelationshipComponent>();
			CalculateEntityWorldPosition(child, true);
			child = Entity(r.Next, this);
		}
	}

	void Scene::CalculateEntityWorldPosition(Entity entity, bool recalculateLocal)
	{
		auto& transform = entity.GetTransform();
		auto& relation = entity.GetComponent<RelationshipComponent>();

		if (!recalculateLocal && IsPhysicsSimulated() && entity.HasComponent<RigidbodyComponent>())
		{
			// Recalculate local position based on world position (physics world)
			SetEntityWorldPosition(entity, transform.WorldPosition, false);
		}
		else
		{
			if (relation.Parent != entt::null)
			{
				auto& parentTransform = Entity(relation.Parent, this).GetTransform();
				transform.WorldPosition = parentTransform.WorldPosition + transform.LocalPosition;
			}
			else
			{
				// Entity is at scene root level
				transform.WorldPosition = transform.LocalPosition;
			}
		}

		// Calculate world positions for child entities
		Entity current{ relation.First, this };
		while (current)
		{
			auto& r = current.GetComponent<RelationshipComponent>();
			CalculateEntityWorldPosition(current, recalculateLocal);
			current = Entity{ r.Next, this };
		}
	}

	void Scene::CalculateWorldPositions(bool recalculateLocal)
	{
		for (auto& entity : m_Root)
			CalculateEntityWorldPosition(entity, recalculateLocal);
	}

	void Scene::SetGameModeByClassName(const std::string& gameModeClassName)
	{
		ScriptFactory::Get().InstantiateGameMode(this, gameModeClassName);
	}

	void Scene::CachePositions()
	{
		CalculateWorldPositions();
		CachePrimaryCameraPosition();
		CacheCursorWorldPosition();
	}

	void Scene::OnUpdate(float ts)
	{
		PROFILE_FUNCTION();

		bool isNetModeClient = m_GameInstance->m_NetworkManager->IsNetModeClient();

		if (m_State == SceneState::Play)
		{
			// Do not update simulation until first replication update
			if (m_EnableNetworking && !m_NetworkInitialized && isNetModeClient)
			{
				CachePositions();
				Camera& camera = GetPrimaryCamera();
				RenderScene(camera);
				return;
			}
		}
		
		if (m_State == SceneState::Play && IsPhysicsSimulated())
		{
			m_PhysicsWorld->ProcessCreatedEntities();
			m_PhysicsTimeAccumulator += ts;
			m_IsPhysicsTick = false;
			
			while (m_PhysicsTimeAccumulator >= m_PhysicsTimestep)
			{
				ScriptsFixedUpdate(m_PhysicsTimestep);
				m_PhysicsWorld->Update(m_PhysicsTimestep);

				m_PhysicsTimeAccumulator -= m_PhysicsTimestep;
				m_IsPhysicsTick = true;
			}
		}

		// Cache positions after physics update
		CachePositions();

		if (m_State == SceneState::Play)
		{
			if (m_NetworkInitialized && isNetModeClient)
			{
				Client* client = m_GameInstance->m_NetworkManager->GetClient();
				client->m_NetTransformSystem->Client_OnUpdate(this, ts);
			}

			if (m_GameMode)
				m_GameMode->OnUpdate(ts);

			ScriptsUpdate(ts);

			auto view = m_Registry.view<SpriteAnimationComponent>();
			for (auto entity : view)
			{
				auto& animation = view.get<SpriteAnimationComponent>(entity).SpriteAnimation;
				animation.Update(ts);
			}
		}

		Camera& camera = GetPrimaryCamera();
		RenderScene(camera);
	}

	void Scene::ScriptsFixedUpdate(float ts)
	{
		ScriptsUpdate(ts, true);
	}

	void Scene::ScriptsUpdate(float ts, bool fixedUpdate)
	{
		PROFILE_FUNCTION();

		auto view = m_Registry.view<ScriptComponent>();
		for (auto entity : view)
		{
			if (!m_Registry.valid(entity))
				continue;

			auto& component = view.get<ScriptComponent>(entity);
			for (auto& script : component.Scripts)
			{
				EntityScript* instance = script.second;

				if (!instance->m_Initialized)
				{
					bool isNetworked = m_Registry.any_of<NetworkComponent>(entity);
					if (isNetworked && m_GameInstance->GetNetworkManager()->IsNetModeClient())
					{
						auto& net = m_Registry.get<NetworkComponent>(entity);
						if (!net.WasReplicated)
							continue; // wait with script initialization 
					}

					instance->m_Initialized = true;
					if (!instance->OnCreate())
					{
						PT_CORE_ERROR("Failed to initialize script {} for an entity {}", script.first, Entity(entity, this).GetUUID());
						instance->m_FailedToInitialize = true;
					}
				}
				
				if (instance->m_FailedToInitialize)
					continue;

				if (fixedUpdate)
					instance->OnFixedUpdate(ts);
				else
					instance->OnUpdate(ts);
			}
		}
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
				return Entity(entity, this);
		}
		return Entity();
	}

	std::vector<Entity> Scene::FindAllByTag(const std::string& tag)
	{
		std::vector<Entity> entities;
		auto view = m_Registry.view<TagComponent>();
		for (auto entity : view) 
		{
			if (tag == view.get<TagComponent>(entity).Tag)
				entities.push_back(Entity(entity, this));
		}
		return entities;
	}

	std::vector<Entity> Scene::FindAllWithScript(const std::string& className)
	{
		std::vector<Entity> entities;
		auto view = m_Registry.view<ScriptComponent>();
		for (auto entity : view)
		{
			auto& script = view.get<ScriptComponent>(entity);
			if (script.Scripts.find(className) != script.Scripts.end())
				entities.push_back(Entity(entity, this));
		}
		return entities;
	}

	Entity Scene::SpawnPrefab(const std::string& prefabPath)
	{
		return PrefabManager::Spawn(this, prefabPath);
	}

	void Scene::RenderScene(const Camera& camera)
	{
		PROFILE_FUNCTION();
		
		Renderer::SetClearColor(m_ClearColor);
		Renderer::Clear();
		Renderer::BeginScene(camera, m_PrimaryCameraPosition);

		// Define camera bounds
		const OrthoProjection& ortho = camera.GetOrthoProjection();
		float cameraLeft = m_PrimaryCameraPosition.x + ortho.Left;
		float cameraRight = m_PrimaryCameraPosition.x + ortho.Right;
		float cameraBottom = m_PrimaryCameraPosition.y + ortho.Bottom;
		float cameraTop = m_PrimaryCameraPosition.y + ortho.Top;

		// Render entities with SpriteComponent
		{
			PROFILE_SCOPE("draw_sprites");
			auto renderableSprite = m_Registry.view<SpriteComponent, TransformComponent>();
			for (auto e : renderableSprite)
			{
				auto [transform, sprite] = renderableSprite.get<TransformComponent, SpriteComponent>(e);
				
				float cosRotation = glm::abs(glm::cos(transform.Rotation));
				float sinRotation = glm::abs(glm::sin(transform.Rotation));

				const auto& scale = transform.Scale;
				float rotatedWidth = (scale.x * cosRotation + scale.y * sinRotation) * 2.0f;
				float rotatedHeight = (scale.y * cosRotation + scale.x * sinRotation) * 2.0f;

				float spriteLeft = transform.WorldPosition.x - rotatedWidth;
				float spriteRight = transform.WorldPosition.x + rotatedWidth;
				float spriteBottom = transform.WorldPosition.y - rotatedHeight;
				float spriteTop = transform.WorldPosition.y + rotatedHeight;

				if (spriteRight < cameraLeft || spriteLeft > cameraRight ||
					spriteTop < cameraBottom || spriteBottom > cameraTop)
				{
					continue;
				}

				// Sprite mirror flip
				glm::vec3 spriteScale = {
					transform.Scale.x * (sprite.Sprite.m_MirrorFlip.x ? -1.0f : 1.0f),
					transform.Scale.y * (sprite.Sprite.m_MirrorFlip.y ? -1.0f : 1.0f), 1.0f
				};

				glm::mat4 transformMatrix = Math::GetTransform(transform.WorldPosition, spriteScale, transform.Rotation);

				if (sprite.Sprite)
					Renderer::DrawQuad(transformMatrix, sprite.Sprite, sprite.Color, sprite.TilingFactor);
				else
					Renderer::DrawQuad(transformMatrix, sprite.Color, sprite.TilingFactor);
			}
		}

		// Render entities with ResizableSpriteComponent
		{
			PROFILE_SCOPE("draw_resizable_sprites");
			auto renderableResizableSprite = m_Registry.view<TransformComponent, ResizableSpriteComponent>();
			for (auto e : renderableResizableSprite)
			{
				auto [transform, rsc] = renderableResizableSprite.get<TransformComponent, ResizableSpriteComponent>(e);
				auto& sprite = rsc.ResizableSprite;

				float cosRotation = std::abs(glm::cos(transform.Rotation));
				float sinRotation = std::abs(glm::sin(transform.Rotation));

				const auto& scale = transform.Scale;
				float rotatedWidth = (scale.x * cosRotation + scale.y * sinRotation) * 2.0f;
				float rotatedHeight = (scale.y * cosRotation + scale.x * sinRotation) * 2.0f;

				float spriteLeft = transform.WorldPosition.x - rotatedWidth;
				float spriteRight = transform.WorldPosition.x + rotatedWidth;
				float spriteBottom = transform.WorldPosition.y - rotatedHeight;
				float spriteTop = transform.WorldPosition.y + rotatedHeight;

				if (spriteRight < cameraLeft || spriteLeft > cameraRight ||
					spriteTop < cameraBottom || spriteBottom > cameraTop)
				{
					continue;
				}

				sprite.Render(Math::GetTransform(transform.WorldPosition, glm::vec2{1.0f}, transform.Rotation), rsc.Color);
			}
		}

		// Render Circles
		auto circlesView = m_Registry.view<TransformComponent, CircleRendererComponent>();
		for (auto entity : circlesView)
		{
			auto [transform, circle] = circlesView.get<TransformComponent, CircleRendererComponent>(entity);

			Renderer::DrawCircle(Math::GetTransform(transform.WorldPosition, transform.Scale, transform.Rotation), circle.Color, circle.Thickness, circle.Fade);
		}

		// Render Text
		auto textView = m_Registry.view<TransformComponent, TextComponent>();
		for (auto entity : textView)
		{
			auto [transform, text] = textView.get<TransformComponent, TextComponent>(entity);
			if (!text.Hidden)
				Renderer::DrawString(text.TextString, transform.WorldPosition, transform.Scale, transform.Rotation, text);
		}

		Renderer::EndScene();
	}

	void Scene::OnViewportResize(uint32_t width, uint32_t height)
	{
		m_ViewportSize = { (float)width, float(height) };
		auto view = m_Registry.view<CameraComponent>();
		for (auto entity : view)
		{
			auto& camera = view.get<CameraComponent>(entity);
			camera.Camera.SetViewportSize(m_ViewportSize);
		}
		m_DefaultCamera.SetViewportSize(m_ViewportSize);
	}

	void Scene::ReleaseGameMode()
	{
		if (m_GameMode)
		{
			delete m_GameMode;
			m_GameMode = nullptr;
		}
	}

	Camera& Scene::GetPrimaryCamera()
	{
	#ifdef PT_EDITOR
		auto viewport = EditorLayer::GetSceneViewportPanel(m_GameInstance);

		if (m_State == SceneState::Stop || viewport->m_Camera->m_UseInRuntime)
			return viewport->m_Camera->GetBaseCamera();
	#endif
		return m_PrimaryCamera ? *m_PrimaryCamera : m_DefaultCamera;
	}

	void Scene::CachePrimaryCameraPosition()
	{
	#ifdef PT_EDITOR
		auto viewport = EditorLayer::GetSceneViewportPanel(m_GameInstance);
		if (m_State == SceneState::Stop || viewport->m_Camera->m_UseInRuntime)
		{
			m_PrimaryCameraPosition = viewport->GetCamera()->GetPosition();
			return;
		}
	#endif
		if (m_PrimaryCameraEntity == entt::null) 
		{
			m_PrimaryCameraPosition = glm::vec3{ 0.0f };
			return;
		}
		auto [transform, camera] = m_Registry.get<TransformComponent, CameraComponent>(m_PrimaryCameraEntity);
		m_PrimaryCameraPosition = glm::vec3 {
			transform.WorldPosition.x + camera.PositionOffset.x,
			transform.WorldPosition.y + camera.PositionOffset.y, 0
		};
	}

	void Scene::CacheCursorWorldPosition()
	{
	#ifdef PT_EDITOR
		auto viewport = EditorLayer::GetSceneViewportPanel(m_GameInstance);
		const uint32_t width = (uint32_t)viewport->m_ViewportSize.x;
		const uint32_t height = (uint32_t)viewport->m_ViewportSize.y;
		const glm::vec2& mouse = viewport->m_MousePos;
	#else
		Window& window = Application::Get().GetWindow();
		const uint32_t width = window.GetWidth();
		const uint32_t height = window.GetHeight();
		const glm::vec2 mouse = Input::GetMousePosition();
	#endif
		OrthoProjection ortho = GetPrimaryCamera().GetOrthoProjection();
		auto& camera = GetPrimaryCameraPosition();
		m_CursorWorldPosition[0] = mouse.x / (float)width * ortho.Right * 2.0f + camera.x + ortho.Left;
		m_CursorWorldPosition[1] = mouse.y / (float)height * ortho.Bottom * 2.0f + camera.y + ortho.Top;
	}

	void Scene::SetPrimaryCameraEntity(Entity entity)
	{
		if (!entity || !entity.HasComponent<CameraComponent>())
		{
			PT_CORE_ERROR("Entity does not have CameraComponent!");
			return;
		}

		auto& camera = entity.GetComponent<CameraComponent>();
		m_PrimaryCamera = &camera.Camera;
		m_PrimaryCameraEntity = entity.m_Handle;
	}

	Entity Scene::GetPrimaryCameraEntity()
	{
		return Entity{ m_PrimaryCameraEntity, this };
	}

	const glm::vec2& Scene::GetCursorWorldPosition()
	{
		return m_CursorWorldPosition;
	}

	bool Scene::IsCursorHoveringEntity(Entity entity)
	{
		auto& transform = entity.GetComponent<TransformComponent>();

		// Apply entity rotation to point (mouse position)
		// so we can easily check after if point is inside rectangle
		float sinus = sin(glm::radians(-transform.Rotation));
		float cosinus = cos(glm::radians(-transform.Rotation));
		glm::vec2 point = GetCursorWorldPosition();
		if (transform.Rotation != 0.0f)
		{
			glm::vec2 rotationCenter = { transform.WorldPosition.x, transform.WorldPosition.y };
			point -= rotationCenter;
			point = {
				point.x * cosinus - point.y * sinus + rotationCenter.x,
				point.x * sinus + point.y * cosinus + rotationCenter.y
			};
		}

		if (m_Registry.any_of<TextComponent>(entity))
		{
			// Handle entities with TextComponent separately
			auto& textComponent = m_Registry.get<TextComponent>(entity);
			glm::vec2 textSize = textComponent.FontAsset->CalculateTextSize(&textComponent, transform.Scale);
			const glm::vec3& textPosition = transform.WorldPosition;

			return point.x >= textPosition.x - textSize.x / 2.0f && point.x <= textPosition.x + textSize.x / 2.0f
				&& point.y >= textPosition.y - textSize.y / 2.0f && point.y <= textPosition.y + textSize.y / 2.0f;
		}
		else
		{
			// Other entitites
			const glm::vec3& position = transform.WorldPosition;
			const glm::vec2& scale = transform.Scale;
			return point.x >= position.x - scale.x / 2.0f && point.x <= position.x + scale.x / 2.0f
				&& point.y >= position.y - scale.y / 2.0f && point.y <= position.y + scale.y / 2.0f;
		}
	}

	std::vector<Entity> Scene::GetEntitiesOnCursorLocation()
	{
		const glm::vec2& mousePos = GetCursorWorldPosition();
		auto view = m_Registry.view<TransformComponent>();
		std::vector<Entity> entities;

		for (auto entity : view)
		{
			auto& transform = view.get<TransformComponent>(entity);
			
			float sinus = sin(glm::radians(-transform.Rotation));
			float cosinus = cos(glm::radians(-transform.Rotation));
			
			// Apply entity rotation to a point (mouse position) so we can easily check if the point is inside the rectangle
			glm::vec2 point = mousePos;
			if (transform.Rotation != 0.0f)
			{
				glm::vec2 rotationCenter = { transform.WorldPosition.x, transform.WorldPosition.y };
				point -= rotationCenter;
				point = {
					point.x * cosinus - point.y * sinus + rotationCenter.x,
					point.x * sinus + point.y * cosinus + rotationCenter.y
				};
			}

			if (m_Registry.any_of<TextComponent>(entity))
			{
				// Handle entities with TextComponent separately
				auto& textComponent = m_Registry.get<TextComponent>(entity);
				glm::vec2 textSize = textComponent.FontAsset->CalculateTextSize(&textComponent, transform.Scale);
				const glm::vec3& textPosition = transform.WorldPosition;

				// Check if the point (mouse position) is inside the text bounding box
				if (point.x >= textPosition.x - textSize.x / 2.0f && point.x <= textPosition.x + textSize.x / 2.0f
					&& point.y >= textPosition.y - textSize.y / 2.0f && point.y <= textPosition.y + textSize.y / 2.0f)
				{
					entities.emplace_back(Entity{ entity, this });
				}
			}
			else
			{
				// Check if the point is inside entity bounding box (non-text entities)
				const glm::vec3& position = transform.WorldPosition;
				const glm::vec2& scale = transform.Scale;
				if (point.x >= position.x - scale.x / 2.0f && point.x <= position.x + scale.x / 2.0f
					&& point.y >= position.y - scale.y / 2.0f && point.y <= position.y + scale.y / 2.0f)
				{
					entities.emplace_back(Entity{ entity, this });
				}
			}
		}

		return entities;
	}

	void Scene::EnablePhysics(bool enabled)
	{
		m_EnablePhysics = enabled;
	}

	const glm::vec3& Scene::GetPrimaryCameraPosition()
	{
		return m_PrimaryCameraPosition;
	}

	void Scene::SetScreenClearColor(const glm::vec4& color)
	{
		m_ClearColor = color;
	}

	NetworkManager* Scene::GetNetworkManager() const
	{
		return m_GameInstance->m_NetworkManager.get();
	}

	uint32_t Scene::GetEntitiesCount() const
	{
		return (uint32_t)m_Registry.view<IDComponent>().size();
	}

	uint32_t Scene::GetScriptedEntitiesCount() const
	{
		return (uint32_t)m_Registry.view<ScriptComponent>().size();
	}

	uint32_t Scene::GetNetworkedEntitiesCount() const
	{
		return (uint32_t)m_Registry.view<NetworkComponent>().size();
	}

	const std::string& Scene::GetFilepath() const
	{
		return m_Filepath;
	}

	bool Scene::IsPhysicsWorldInitialized() const
	{
		return m_PhysicsWorld->IsInitialized();
	}

	bool Scene::IsPhysicsSimulated() const
	{
		return IsPhysicsEnabled() && IsPhysicsWorldInitialized();
	}

	void Scene::SetPhysicsTickrate(float tickrate)
	{
		m_PhysicsTickrate = tickrate;
		m_PhysicsTimestep = 1.0f / tickrate;
	}

	template<typename TComponent>
	void Scene::OnComponentAdded(Entity entity, TComponent& component)
	{
		static_assert(sizeof(T) == 0); // missing OnComponentAdded<T> definition
	}

	template<>
	void Scene::OnComponentAdded<IDComponent>(Entity entity, IDComponent& component)
	{
	}
	
	template<>
	void Scene::OnComponentAdded<TagComponent>(Entity entity, TagComponent& component)
	{
	}

	template<>
	void Scene::OnComponentAdded<TransformComponent>(Entity entity, TransformComponent& component)
	{
	}

	template<>
	void Scene::OnComponentAdded<RelationshipComponent>(Entity entity, RelationshipComponent& component)
	{
	}
	
	template<>
	void Scene::OnComponentAdded<PrefabComponent>(Entity entity, PrefabComponent& component)
	{
	}

	template<>
	void Scene::OnComponentAdded<CameraComponent>(Entity entity, CameraComponent& component)
	{
		if (m_ViewportSize.x > 0.0f && m_ViewportSize.y > 0.0f)
			component.Camera.SetViewportSize(m_ViewportSize);
	}

	template<>
	void Scene::OnComponentAdded<SpriteComponent>(Entity entity, SpriteComponent& component)
	{
	}

	template<>
	void Scene::OnComponentAdded<ResizableSpriteComponent>(Entity entity, ResizableSpriteComponent& component)
	{
	}

	template<>
	void Scene::OnComponentAdded<CircleRendererComponent>(Entity entity, CircleRendererComponent& component)
	{
	}

	template<>
	void Scene::OnComponentAdded<TextComponent>(Entity entity, TextComponent& component)
	{
	}

	template<>
	void Scene::OnComponentAdded<SpriteAnimationComponent>(Entity entity, SpriteAnimationComponent& component)
	{
		component.SpriteAnimation.m_OwningEntity = MakeUnique<Entity>(entity, this);
	}

	template<>
	void Scene::OnComponentAdded<ScriptComponent>(Entity entity, ScriptComponent& component)
	{
	}

	template<>
	void Scene::OnComponentAdded<RigidbodyComponent>(Entity entity, RigidbodyComponent& component)
	{

		if (m_State == SceneState::Play && m_PhysicsWorld->IsInitialized())
			m_PhysicsWorld->m_EntitiesToInitialize.push_back(entity);
	}

	template<>
	void Scene::OnComponentAdded<BoxColliderComponent>(Entity entity, BoxColliderComponent& component)
	{
		if (m_State == SceneState::Play && m_PhysicsWorld->IsInitialized())
			m_PhysicsWorld->m_EntitiesToInitialize.push_back(entity);
	}

	template<>
	void Scene::OnComponentAdded<CircleColliderComponent>(Entity entity, CircleColliderComponent& component)
	{
		if (m_State == SceneState::Play && m_PhysicsWorld->IsInitialized())
			m_PhysicsWorld->m_EntitiesToInitialize.push_back(entity);
	}

	template<>
	void Scene::OnComponentAdded<NetworkComponent>(Entity entity, NetworkComponent& component)
	{
		auto& transform = entity.GetTransform();
		auto& netTransform = component.NetTransform;
		auto& last = netTransform.LastAuthoritativeTransform;
		auto& prev = netTransform.PrevAuthoritativeTransform;

		bool rigidbodySimulated = component.SimulateOnClient && entity.HasComponent<RigidbodyComponent>();
		last.Position = entity.HasComponent<RigidbodyComponent>() ? transform.WorldPosition : transform.LocalPosition;
		last.Scale = transform.Scale;
		last.Rotation = transform.Rotation;

		prev = last;
		netTransform.LastTickTransform = last;
	}

}
