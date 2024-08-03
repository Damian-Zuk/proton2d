#pragma once

#include "Proton/Graphics/Camera.h"
#include "Proton/Events/Event.h"
#include "Proton/Core/UUID.h"
#include "Proton/Network/NetSyncData.h"

#include <entt/entt.hpp>

class b2Body;

namespace proton {

	constexpr glm::vec4 DEFAULT_SCENE_SCREEN_CLEAR_COLOR = { 0.24f, 0.37f, 0.67f, 1.0f };

	// Forward declaration
	class Entity;
	class PhysicsWorld;
	class GameInstance;
	class NetworkManager;
	class GameModeBase;

	enum class SceneState
	{
		Stop, Play, Paused
	};

	//
	// Scene Class
	// Wrapper for the Entity Registry (entt:registry) from the EnTT Entity Component System (ECS) library.
	// Provides additional functionalities to manage entities on the scene.
	//
	class Scene
	{
	public:
		Scene(const std::string& name = "Unnamed scene",
			const std::string& filepath = "",
			const std::string& gameModeClass = "");

		virtual ~Scene();

		Shared<Scene> CreateSceneCopy(GameInstance* gameInstance = nullptr);

		void BeginPlay(); // SceneState::Play
		void Pause(bool pause = true); // SceneState::Pause
		void Stop(); // SceneState::Stop
		
		Entity CreateEntity(const std::string& name = "Entity");
		Entity CreateEntityWithUUID(UUID id, const std::string& name = "Entity", bool addToSceneRoot = true);
		Entity DuplicateEntity(Entity entity, Entity attachTo);
		void DestroyEntity(Entity entity, bool popHierachy = true);
		void DestroyChildEntities(Entity entity);
		void DestroyAll();

		void SetEntityLocalPosition(Entity entity, const glm::vec3& position, bool recalculateChildren = true);
		void SetEntityWorldPosition(Entity entity, const glm::vec3& position, bool recalculateChildren = true);

		Entity FindByID(UUID id);
		Entity FindByTag(const std::string& tag);
		std::vector<Entity> FindAllByTag(const std::string& tag);

		void SetPrimaryCameraEntity(Entity entity);
		Entity GetPrimaryCameraEntity();
		Camera& GetPrimaryCamera();
		const glm::vec3& GetPrimaryCameraPosition();

		const glm::vec2& GetCursorWorldPosition();
		bool IsCursorHoveringEntity(Entity entity);
		std::vector<Entity> GetEntitiesOnCursorLocation();

		void EnablePhysics(bool enabled);
		bool IsPhysicsEnabled() const;
		bool IsPhysicsWorldInitialized() const;
		bool IsPhysicsSimulated() const;
		bool IsSimulated() const { return m_SceneState != SceneState::Stop; };
		bool IsPaused() const { return m_SceneState == SceneState::Paused; };

		const std::string& GetFilepath() const;
		SceneState GetSceneState() const { return m_SceneState; }
		uint32_t GetEntitiesCount() const;
		uint32_t GetScriptedEntitiesCount() const;
		uint32_t GetNetworkedEntitiesCount() const;

		void SetScreenClearColor(const glm::vec4& color);

		template<typename... Components>
		auto GetAllEntitiesWith() { return m_Registry.view<Components...>(); }

		GameInstance* GetOwningGameInstance() const { return m_GameInstance; }
		NetworkManager* GetNetworkManager() const;

		template<typename TGameMode>
		GameModeBase* SetGameMode()
		{
			ReleaseGameMode();
			m_GameModeClassName = TGameMode::__ClassName;
			m_GameMode = new TGameMode();
			m_GameMode->m_Scene = this;
			return m_GameMode;
		}

		void SetGameModeByClassName(const std::string& gameModeClassName);

		template<typename TGameMode>
		TGameMode* GameModeCastTo()
		{
			return dynamic_cast<TGameMode*>(m_GameMode);
		}

		GameModeBase* GetGameMode() const { return m_GameMode; }

	private:
		void OnUpdate(float ts);
		void UpdateScripts(float ts);
		void RenderScene(const Camera& camera);
		void RenderUI();

		void OnViewportResize(uint32_t width, uint32_t height);
		void ReleaseGameMode();

		void CachePrimaryCameraPosition();
		void CacheCursorWorldPosition();

		void CalculateEntityWorldPosition(Entity entity, bool recalculateLocal = false);
		void CalculateWorldPositions(bool recalculateLocal = false);

		void BuildPhysicsWorld();

		template<typename T>
		void OnComponentAdded(Entity entity, T& component);

	private:
		SceneState m_SceneState = SceneState::Stop;
		GameInstance* m_GameInstance = nullptr;
		
		GameModeBase* m_GameMode = nullptr;
		std::string m_GameModeClassName = "GameModeBase";

		// General
		std::string m_SceneName;
		std::string m_SceneFilepath = "<Unsaved scene>";
		glm::vec4 m_ClearColor = DEFAULT_SCENE_SCREEN_CLEAR_COLOR;

		// ECS
		entt::registry m_Registry;
		std::unordered_map<UUID, Entity> m_EntityMap;
		std::vector<Entity> m_Root;

		// Camera
		glm::vec2 m_ViewportSize = { 0.0f, 0.0f };
		entt::entity m_PrimaryCameraEntity = entt::null;
		Camera* m_PrimaryCamera = nullptr;
		Camera m_DefaultCamera;

		// Physics
		bool m_EnablePhysics = true;
		Unique<PhysicsWorld> m_PhysicsWorld;
		float m_PhysicsTimestep = 0.01f;
		float m_PhysicsTimer = 0.0f;
		bool m_PhysicsTick = false;

		// Cache
		glm::vec3 m_PrimaryCameraPosition = { 0.0f, 0.0f, 0.0f };
		glm::vec2 m_CursorWorldPosition = { 0.0f, 0.0f };

		// Network
		bool m_EnableNetworking = true;

		friend class Application;
		friend class Entity;
		friend class EntityScript;
		friend class SceneSerializer;
		friend class SceneManager;
		friend class PrefabManager;
		friend class PhysicsWorld;
		friend class GameInstance;
		friend class GameModeBase;
		friend class NetworkManager;
		friend class NetSyncSystem;
		friend class Server;
		friend class Client;
		
		friend class EditorLayer;
		friend class EditorCamera;
		friend class SettingsPanel;
		friend class InspectorPanel;
		friend class SceneHierarchyPanel;
		friend class ToolbarPanel;
		friend class EditorMenuBar;
		friend class SceneViewportPanel;
	};

}
