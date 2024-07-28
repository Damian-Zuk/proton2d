#include "ptpch.h"
#include "Proton/Scene/Scene.h"
#include "Proton/Scene/Entity.h"
#include "Proton/Graphics/Renderer/Renderer.h"
#include "Proton/Scripting/EntityScript.h"
#include "Proton/Scripting/GameModeBase.h"
#include "Proton/Scripting/GameModeFactory.h"
#include "Proton/Core/Application.h"
#include "Proton/Core/GameInstance.h"
#include "Proton/Core/Input.h"
#include "Proton/Assets/SceneSerializer.h"
#include "Proton/Utils/Utils.h"
#include "Proton/Physics/PhysicsWorld.h"
#include "Proton/UI/UIElement.h"

#include "Proton/Network/Common/NetworkManager.h"
#include "Proton/Network/Client/NetSyncSystem.h"
#include "Proton/Network/Server/Server.h"

#ifdef PT_EDITOR
#include "Proton/Editor/EditorLayer.h"
#include "Proton/Editor/Panels/SceneViewportPanel.h"
#include <imgui.h>
#endif

namespace proton {

	Scene::Scene(const std::string& name,
		const std::string& filepath, 
		const std::string& gameModeClass)
		: m_SceneName(name), m_SceneFilepath(filepath),
		  m_PhysicsWorld(MakeUnique<PhysicsWorld>(this))
	{
		if (gameModeClass.length() && gameModeClass != "GameModeBase")
		{
			m_GameModeClassName = gameModeClass;
			GameModeFactory::Get().InstantiateGameMode(this, m_GameModeClassName);
		}
		else
			m_GameMode = new GameModeBase();

		m_NetDefaultSyncParams.SyncMethod = NetSyncMethod::NetworkRigidbody;
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
				delete script.second;
		}
	}

	/////////////////////////////////////////////////////////////////////////////////////////////
	// CopyComponent function implementations taken from:
	// https://github.com/TheCherno/Hazel/blob/master/Hazel/src/Hazel/Scene/Scene.cp
	/////////////////////////////////////////////////////////////////////////////////////////////
	template<typename... Component>
	struct ComponentGroup
	{
	};

	using ComponentsToCopy =
		ComponentGroup<TransformComponent, CameraComponent, VelocityComponent,
		SpriteComponent, CircleRendererComponent, ResizableSpriteComponent,
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
	/////////////////////////////////////////////////////////////////////////////////////////////

	Shared<Scene> Scene::CreateSceneCopy(GameInstance* gameInstance)
	{
		Shared<Scene> newScene = MakeShared<Scene>(m_SceneName, m_SceneFilepath, m_GameModeClassName);
		newScene->m_ClearColor = m_ClearColor;
		newScene->m_EnablePhysics = m_EnablePhysics;
		newScene->m_PhysicsTimestep = m_PhysicsTimestep;

		newScene->m_InheritNetMode = m_InheritNetMode;
		newScene->m_NetDefaultSyncParams = m_NetDefaultSyncParams;

		if (gameInstance)
			newScene->m_GameInstance = gameInstance;
		else	
			newScene->m_GameInstance = m_GameInstance;

		auto& dstSceneRegistry = newScene->m_Registry;
		std::unordered_map<UUID, entt::entity> enttMap;

		// Create entities in new scene
		for (entt::entity srcEntity : m_Registry.view<IDComponent>())
		{
			UUID uuid = m_Registry.get<IDComponent>(srcEntity).ID;
			const auto& name = m_Registry.get<TagComponent>(srcEntity).Tag;
			Entity newEntity = newScene->CreateEntityWithUUID(uuid, name, false);
			enttMap[uuid] = (entt::entity)newEntity;
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
					script->SetFieldValueData(fieldName, fieldData.InstanceFieldValue);
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
		if (m_SceneState == SceneState::Play || m_SceneState == SceneState::Paused)
			return; 

	#ifdef PT_EDITOR
		EditorLayer::Get()->OnBeginSceneSimulation(this);
	#endif

		//NetMode netMode = m_GameInstance->GetNetMode();
		//if (m_NetDefaultSyncParams.SyncMethod != NetSyncMethod::NetworkRigidbody 
		//	&& netMode == NetMode::Client && m_InheritNetMode)
		//	m_EnablePhysics = false;

		m_SceneState = SceneState::Play;
		m_GameInstance->OnSceneSimulationStart(this);
		
		CalculateWorldPositions();

		if (m_EnablePhysics)
			BuildPhysicsWorld();

		if (m_GameMode)
			m_GameMode->OnCreate();

		m_PhysicsTimer = 0.0f;
	}


	void Scene::BuildPhysicsWorld()
	{
		if (!m_PhysicsWorld->IsInitialized())
			m_PhysicsWorld->BuildWorld();
	}

	void Scene::Pause(bool pause)
	{
		if (m_SceneState == SceneState::Stop)
			return;

		m_SceneState = pause ? SceneState::Paused : SceneState::Play;
	}

	void Scene::Stop()
	{
		if (m_SceneState == SceneState::Stop)
			return;

		if (m_PhysicsWorld->IsInitialized())
			m_PhysicsWorld->DestroyWorld();

		m_GameMode->OnDestroy();
		m_GameMode = GameModeFactory::Get().InstantiateGameMode(this, m_GameModeClassName);

		m_SceneState = SceneState::Stop;
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
		if (networkManager->IsNetServiceRunning() && networkManager->IsNetModeServer())
		{
			networkManager->GetServer()->PushCreatedEntity(id, this);
		}

		return entity;
	}

	Entity Scene::DuplicateEntity(Entity entity, Entity attachTo)
	{
		Entity newEntity = CreateEntity(entity.GetTag());

		auto& srcRelation = entity.GetComponent<RelationshipComponent>();
		auto& dstRelation = newEntity.GetComponent<RelationshipComponent>();

		if (attachTo)
		{
			attachTo.AddChildEntity(newEntity);
		}
		else if (srcRelation.Parent != entt::null)
		{
			Entity(srcRelation.Parent, this).AddChildEntity(newEntity);
		}
		else
			m_Root.push_back(newEntity);

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
					script->SetFieldValueData(fieldName, fieldData.InstanceFieldValue);
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

		// If server is running and entity has NetworkComponent
		// inform clients that entity has been destroyed.
		NetworkManager* networkManager = m_GameInstance->GetNetworkManager();
		if (networkManager->IsNetServiceRunning() && networkManager->IsNetModeServer()
			&& entity.HasComponent<NetworkComponent>())
		{
			networkManager->GetServer()->PushDestroyedEntity(entity.GetUUID(), this);
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
		GameModeFactory::Get().InstantiateGameMode(this, gameModeClassName);
	}

	void Scene::OnUpdate(float ts)
	{
		PROFILE_FUNCTION();
		
		if (m_SceneState == SceneState::Play && IsPhysicsSimulated())
		{
			m_PhysicsWorld->ProcessCreatedEntities();
			m_PhysicsTimer += ts;
			m_PhysicsTick = (bool)(m_PhysicsTimer >= m_PhysicsTimestep);
				
			if (m_PhysicsTick)
			{
				if (m_GameInstance->m_NetworkManager->IsNetModeClient())
					NetSyncSystem::UpdatePhysics(this, m_PhysicsTimer);

				m_PhysicsWorld->Update(m_PhysicsTimer);
			}
		}

		CalculateWorldPositions();
		CachePrimaryCameraPosition();
		CacheCursorWorldPosition();

		if (m_SceneState == SceneState::Play)
		{
			if (m_GameMode)
				m_GameMode->OnUpdate(ts);

			UpdateScripts(ts);

			if (m_PhysicsTick)
				m_PhysicsTimer = 0.0f;

			auto view = m_Registry.view<SpriteAnimationComponent>();
			for (auto entity : view)
			{
				auto& animation = view.get<SpriteAnimationComponent>(entity).SpriteAnimation;
				animation.Update(ts);
			}
		}

		Camera& camera = GetPrimaryCamera();
		RenderScene(camera);
		RenderUI();
	}

	void Scene::UpdateScripts(float ts)
	{
		PROFILE_FUNCTION();

		NetworkManager* networkManager = m_GameInstance->m_NetworkManager.get();
		if (networkManager->IsNetModeClient() && !networkManager->m_ClientGameStateInitialized)
			return;// Update scripts after client state initialized

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
					instance->m_Initialized = true;

					if (!instance->OnCreate())
						instance->m_Stopped = true;
				}
				
				if (instance->m_Stopped)
					continue;

				if (m_PhysicsTick && m_Registry.any_of<RigidbodyComponent>(entity)
					&& m_Registry.get<RigidbodyComponent>(entity).RuntimeBody)
				{
					instance->OnPhysicsUpdate(m_PhysicsTimer);
				}

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
		auto view = m_Registry.view<TagComponent>();
		std::vector<Entity> entities;
		entities.reserve(view.size());
		for (auto entity : view) 
		{
			if (tag == view.get<TagComponent>(entity).Tag)
				entities.emplace_back(Entity(entity, this));
		}
		return entities;
	}

	void Scene::RenderScene(const Camera& camera)
	{
		PROFILE_FUNCTION();
		auto campos = GetPrimaryCameraPosition();
		Renderer::BeginScene(camera, campos);

		// Render entities with SpriteComponent
		auto renderableSprite = m_Registry.view<SpriteComponent, TransformComponent>();
		for (auto e : renderableSprite)
		{
			PROFILE_SCOPE("entity_render_sprite");
			auto [transform, sprite] = renderableSprite.get<TransformComponent, SpriteComponent>(e);
			
			// Sprite mirror flip
			glm::vec3 scale = {
				transform.Scale.x * (sprite.Sprite.m_MirrorFlip.x ? -1.0f : 1.0f),
				transform.Scale.y * (sprite.Sprite.m_MirrorFlip.y ? -1.0f : 1.0f), 1.0f
			};

			glm::mat4 transformMatrix = Math::GetTransform(transform.WorldPosition, scale, transform.Rotation);

			if (sprite.Sprite)
				Renderer::DrawQuad(transformMatrix, sprite.Sprite, sprite.Color, sprite.TilingFactor);
			else
				Renderer::DrawQuad(transformMatrix, sprite.Color, sprite.TilingFactor);
		}

		// Render entities with ResizableSpriteComponent
		auto renderableResizableSprite = m_Registry.view<TransformComponent, ResizableSpriteComponent>();
		for (auto e : renderableResizableSprite)
		{
			PROFILE_SCOPE("entity_render_resizable_sprite");
			auto [transform, rsc] = renderableResizableSprite.get<TransformComponent, ResizableSpriteComponent>(e);
			auto& sprite = rsc.ResizableSprite;

			sprite.Render(Math::GetTransform(transform.WorldPosition, transform.Scale, transform.Rotation), rsc.Color);
		}

		// Render Circles
		auto circlesView = m_Registry.view<TransformComponent, CircleRendererComponent>();
		for (auto entity : circlesView)
		{
			PROFILE_SCOPE("entity_render_circle");
			auto [transform, circle] = circlesView.get<TransformComponent, CircleRendererComponent>(entity);

			Renderer::DrawCircle(Math::GetTransform(transform.WorldPosition, transform.Scale, transform.Rotation), circle.Color, circle.Thickness, circle.Fade);
		}

		// Render Text
		auto textView = m_Registry.view<TransformComponent, TextComponent>();
		for (auto entity : textView)
		{
			PROFILE_SCOPE("entity_render_text");
			auto [transform, text] = textView.get<TransformComponent, TextComponent>(entity);
			if (!text.Hidden)
				Renderer::DrawString(text.TextString, Math::GetTransform(transform.WorldPosition, transform.Scale, transform.Rotation), text);
		}

		Renderer::EndScene();
	}

	void Scene::RenderUI()
	{
		PROFILE_FUNCTION();

		Renderer::BeginScene(GetPrimaryCamera().GetAspectRatio());

		auto view = m_Registry.view<TransformComponent, UITextComponent>();
		for (auto entity : view)
		{
			auto [transform, ui] = view.get<TransformComponent, UITextComponent>(entity);
			if (ui.UIText.IsHidden())
				continue;

			ui.UIText.Draw(&transform);
		}

		Renderer::EndScene();
	}

	void Scene::OnViewportResize(uint32_t width, uint32_t height)
	{
		auto view = m_Registry.view<CameraComponent>();
		for (auto entity : view)
		{
			auto& camera = view.get<CameraComponent>(entity);
			camera.Camera.SetAspectRatio((float)width / float(height));
		}
		m_DefaultCamera.SetAspectRatio((float)width / float(height));
	}

	void Scene::ReleaseGameMode()
	{
		if (m_GameMode)
			delete m_GameMode;
		m_GameMode = nullptr;
	}

	void Scene::CachePrimaryCameraPosition()
	{
	#ifdef PT_EDITOR
		if (m_GameInstance->m_IsMainInstance && (m_SceneState == SceneState::Stop || EditorLayer::GetCamera()->m_UseInRuntime))
		{
			m_PrimaryCameraPosition = EditorLayer::GetCamera()->GetPosition();
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
		uint32_t width = (uint32_t)EditorLayer::GetSceneViewportPanel()->m_ViewportSize.x;
		uint32_t height = (uint32_t)EditorLayer::GetSceneViewportPanel()->m_ViewportSize.y;
		const glm::vec2& mouse = EditorLayer::GetSceneViewportPanel()->m_MousePos;
	#else
		Window& window = Application::Get().GetWindow();
		uint32_t width = window.GetWidth();
		uint32_t height = window.GetHeight();
		glm::vec2 mouse = Input::GetMousePosition();
	#endif
		OrthoProjection ortho = GetPrimaryCamera().GetOrthoProjection();
		auto& camera = GetPrimaryCameraPosition();
		m_CursorWorldPosition[0] = mouse.x / (float)width * ortho.Right * 2.0f + camera.x + ortho.Left;
		m_CursorWorldPosition[1] = mouse.y / (float)height * ortho.Bottom * 2.0f + camera.y + ortho.Top;
	}

	Camera& Scene::GetPrimaryCamera()
	{
	#ifdef PT_EDITOR
		if (m_GameInstance->m_IsMainInstance && (m_SceneState == SceneState::Stop || EditorLayer::GetCamera()->m_UseInRuntime))
			return EditorLayer::GetCamera()->GetBaseCamera();
	#endif
		return m_PrimaryCamera ? *m_PrimaryCamera : m_DefaultCamera;
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
		// so we can easliy check after if point is inside rectangle
		float sinus = sin(glm::radians(-transform.Rotation));
		float cosinus = cos(glm::radians(-transform.Rotation));
		glm::vec2 point = GetCursorWorldPosition();
		if (transform.Rotation)
		{
			glm::vec2 rotationCenter = { transform.WorldPosition.x, transform.WorldPosition.y };
			point -= rotationCenter;
			point = {
				point.x * cosinus - point.y * sinus + rotationCenter.x,
				point.x * sinus + point.y * cosinus + rotationCenter.y
			};
		}

		// Check if point is inside entity bounding box
		const glm::vec3& position = transform.WorldPosition;
		const glm::vec2& scale = transform.Scale;
		return point.x >= position.x - scale.x / 2.0f && point.x <= position.x + scale.x / 2.0f
			&& point.y >= position.y - scale.y / 2.0f && point.y <= position.y + scale.y / 2.0f;
	}

	std::vector<Entity> Scene::GetEntitiesOnCursorLocation()
	{
		// TODO: Optimize / refactor
		const glm::vec2& mousePos = GetCursorWorldPosition();
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
				glm::vec2 rotationCenter = { transform.WorldPosition.x, transform.WorldPosition.y };
				point -= rotationCenter;
				point = {
					point.x * cosinus - point.y * sinus + rotationCenter.x,
					point.x * sinus + point.y * cosinus + rotationCenter.y
				};
			}

			// Check if point is inside entity bounding box
			const glm::vec3& position = transform.WorldPosition;
			const glm::vec2& scale = transform.Scale;
			if (point.x >= position.x - scale.x / 2.0f && point.x <= position.x + scale.x / 2.0f
				&& point.y >= position.y - scale.y / 2.0f && point.y <= position.y + scale.y / 2.0f)
			{
				entities.emplace_back(Entity{ entity, this });
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
		return m_SceneFilepath;
	}

	bool Scene::IsPhysicsEnabled() const
	{
		return m_EnablePhysics;
	}

	bool Scene::IsPhysicsWorldInitialized() const
	{
		return m_PhysicsWorld->IsInitialized();
	}

	bool Scene::IsPhysicsSimulated() const
	{
		return IsPhysicsEnabled() && IsPhysicsWorldInitialized();
	}
}
