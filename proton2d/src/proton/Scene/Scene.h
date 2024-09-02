#pragma once

#include "Proton/Graphics/Camera.h"
#include "Proton/Events/Event.h"
#include "Proton/Core/UUID.h"

#include <entt/entt.hpp>

class b2Body;

namespace proton {

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

	////////////////////////////////////////////////////////////////////////////////////////
	// Scene class (wrapper around entt:registry with functionalities to manage entities) 
	////////////////////////////////////////////////////////////////////////////////////////
	class Scene
	{
	public:
		Scene(const std::string& filepath = "", const std::string& gameModeClass = "");

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

		Entity SpawnPrefab(const std::string& prefabPath);

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
		bool IsPhysicsTick() const { return m_IsPhysicsTick; }
		float GetPhysicsTimestep() const { return m_PhysicsTimestep; }

		bool IsSimulated() const { return m_State != SceneState::Stop; };
		bool IsPaused() const { return m_State == SceneState::Paused; };
		SceneState GetSceneState() const { return m_State; }
		const std::string& GetFilepath() const;
		uint32_t GetEntitiesCount() const;
		uint32_t GetScriptedEntitiesCount() const;
		uint32_t GetNetworkedEntitiesCount() const;

		void SetScreenClearColor(const glm::vec4& color);

		template<typename... Components>
		auto GetAllEntitiesWith() { return m_Registry.view<Components...>(); }

		GameInstance* GetGameInstance() const { return m_GameInstance; }
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
		GameModeBase* GetGameMode() const { return m_GameMode; }

	private:
		void OnUpdate(float ts);
		void UpdateScripts(float ts, bool updatePhysics = false);
		void RenderScene(const Camera& camera);
		void RenderUI();

		void OnViewportResize(uint32_t width, uint32_t height);
		void ReleaseGameMode();

		void CachePrimaryCameraPosition();
		void CacheCursorWorldPosition();

		void CalculateEntityWorldPosition(Entity entity, bool recalculateLocal = false);
		void CalculateWorldPositions(bool recalculateLocal = false);

		void BuildPhysicsWorld();

		template<typename TComponent>
		void OnComponentAdded(Entity entity, TComponent& component);

	private:
		SceneState m_State = SceneState::Stop;
		GameInstance* m_GameInstance = nullptr;
		
		GameModeBase* m_GameMode = nullptr;
		std::string m_GameModeClassName = "GameModeBase";

		// General
		std::string m_Filepath = "<Unsaved scene>";
		glm::vec4 m_ClearColor;

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
		bool m_IsPhysicsTick = false;

		// Cache
		glm::vec3 m_PrimaryCameraPosition = { 0.0f, 0.0f, 0.0f };
		glm::vec2 m_CursorWorldPosition = { 0.0f, 0.0f };

		// Network
		bool m_EnableNetworking = true; // If false, scene will run in NetMode::Standalone 
		bool m_NetworkInitialized = false; // True if client has received any replication update from the server

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
		friend class NetTransformSystem;
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
