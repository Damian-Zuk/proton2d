#pragma once

#include "Proton/Core/UUID.h"
#include "Proton/Graphics/Camera.h"
#include "Proton/Events/Event.h"

#include <entt/entt.hpp>

class b2Body;

namespace proton {

	// Forward declaration
	class Entity;
	class PhysicsWorld;
	class GameInstance;
	class NetworkManager;
	class GameScript;

	enum class SceneState
	{
		Stop, Play, Paused
	};

	class Scene
	{
	public:
		Scene(GameInstance* gameInstance, const std::string& filepath = "", const std::string& gameScriptClass = "GameScriptDefault");
		virtual ~Scene();


		template<typename TGameScript>
		GameScript* SetGameScript()
		{
			//static_assert(std::is_base_of(GameScript, TGameScript)::value);
			if (m_GameScript && m_GameScript->m_Status == ScriptStatus::Initialized)
				m_GameScript->OnDestroy();
			delete m_GameScript;
			m_GameScript = new TGameScript();
			m_GameScript->m_GameInstance = m_GameInstance;
			m_GameScript->m_Scene = this;
			return m_GameScript;
		}

		GameScript* SetGameScript(const std::string& className);
		GameScript* GetGameScript() { return m_GameScript; }

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
		std::vector<Entity> FindAllWithScript(const std::string& className);

		template<typename TScriptClass>
		std::vector<Entity> FindAllWithScript()
		{
			return FindAllWithScript(TScriptClass::__ScriptClassName);
		}

		template<typename TScriptClass>
		std::vector<TScriptClass*> FindAllScripts()
		{
			std::vector<TScriptClass*> entities;
			auto view = m_Registry.view<ScriptComponent>();
			for (auto entity : view)
			{
				auto& script = view.get<ScriptComponent>(entity);
				auto it = script.Scripts.find(TScriptClass::__ScriptClassName);
				if (it != script.Scripts.end())
					entities.push_back(dynamic_cast<TScriptClass*>(it->second));
			}
			return entities;
		}

		Entity SpawnPrefab(const std::string& prefabPath);

		void SetPrimaryCameraEntity(Entity entity);
		Entity GetPrimaryCameraEntity();
		Camera& GetPrimaryCamera();
		const glm::vec3& GetPrimaryCameraPosition();

		const glm::vec2& GetCursorWorldPosition();
		bool IsCursorHoveringEntity(Entity entity);
		std::vector<Entity> GetEntitiesOnCursorLocation();

		void EnablePhysics(bool enabled);
		bool IsPhysicsWorldInitialized() const;
		bool IsPhysicsSimulated() const;
		bool IsPhysicsEnabled() const { return m_EnablePhysics; }
		bool IsPhysicsTick() const { return m_IsPhysicsTick; }
		float GetPhysicsTickrate() const { return m_PhysicsTickrate; }
		float GetPhysicsTimestep() const { return m_PhysicsTimestep; }

		void SetPhysicsTickrate(float tickrate);

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

	private:
		void OnUpdate(float ts);
		void ScriptsFixedUpdate(float ts);
		void ScriptsUpdate(float ts, bool fixedUpdate = false);
		void RenderScene(const Camera& camera);

		void OnViewportResize(uint32_t width, uint32_t height);

		void CachePositions();
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
		GameScript* m_GameScript = nullptr;

		// General
		std::string m_Filepath = "<Unsaved scene>";
		glm::vec4 m_ClearColor;

		// EnTT
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
		
		// Physics timestep
		float m_PhysicsTickrate = 120;
		float m_PhysicsTimestep = 1.0f / m_PhysicsTickrate;
		float m_PhysicsTimeAccumulator = 0.0f;
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
