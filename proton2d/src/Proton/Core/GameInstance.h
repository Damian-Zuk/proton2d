#pragma once
#include "Proton/Core/ProjectConfig.h"
#include "Proton/Network/Common.h"

namespace proton {

	class Scene;
	class SceneManager;
	class NetworkManager;
	class AppScript;
	struct EditorGameInstance;

	class GameInstance
	{
	public:
		GameInstance(const std::string& appScriptClassName = "AppScriptDefault");
		virtual ~GameInstance();

		void Init(bool loadStartScene = true);
		void OnUpdate(float ts);

		void OnSceneSimulationStart(Scene* scene);
		void OnSceneSimulationStop(Scene* scene);

		template<typename TAppScript>
		AppScript* SetAppScript()
		{
			static_assert(std::is_base_of<AppScript, TAppScript>::value);
			if (m_AppScript && m_AppScript->m_Status == ScriptStatus::Initialized)
				m_AppScript->OnDestroy();
			delete m_AppScript;
			m_AppScript = new TAppScript();
			m_AppScript->m_GameInstance = this;
			m_AppScript->OnPreInit();
			return m_AppScript;
		}

		AppScript* SetAppScript(const std::string& className);
		AppScript* GetAppScript() { return m_AppScript; }

		Scene* GetActiveScene();

		SceneManager* GetSceneManager();
		NetworkManager* GetNetworkManager();

		void SetNetMode(NetMode mode);
		NetMode GetNetMode() const;

		bool IsMainInstance() const { return m_IsMainInstance; }
		bool HasSimulationStarted() const { return m_SimulatedScenesCount > 0; }

	private:
	#ifdef PT_EDITOR
		EditorGameInstance* m_EditorGameInstance = nullptr;
	#endif

		Unique<SceneManager> m_SceneManager;
		Unique<NetworkManager> m_NetworkManager;

		bool m_IsMainInstance = true;
		ProjectConfig m_ProjectConfig;
		uint32_t m_SimulatedScenesCount = 0;

		AppScript* m_AppScript = nullptr;

		friend class Application;
		friend class SceneManager;
		friend class ScriptFactory;
		friend class Scene;
		friend class Client;
		friend class Server;

		friend class EditorLayer;
		friend class InspectorPanel;
		friend class SceneViewportPanel;
		friend class SettingsPanel;
		friend class EditorMenuBar;
	};

}
