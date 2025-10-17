#pragma once
#include "Proton/Core/ProjectConfig.h"
#include "Proton/Network/Common.h"

namespace proton {

	class Scene;
	class SceneManager;
	class NetworkManager;
	struct EditorGameInstance;

	class GameInstance
	{
	public:
		GameInstance();
		virtual ~GameInstance() = default;

		void Init(bool loadStartScene = true);
		void OnUpdate(float ts);

		void OnSceneSimulationStart(Scene* scene);
		void OnSceneSimulationStop(Scene* scene);

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

		ProjectConfig m_ProjectConfig;
		uint32_t m_SimulatedScenesCount = 0;

		bool m_IsMainInstance = true;

		friend class Application;
		friend class SceneManager;
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
