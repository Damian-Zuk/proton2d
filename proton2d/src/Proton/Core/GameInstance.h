#pragma once
#include "Proton/Core/ProjectSettings.h"
#include "Proton/Network/Common/Common.h"

namespace proton {

	class Scene;
	class SceneManager;
	class NetworkManager;
	class SceneViewportPanel;

	class GameInstance
	{
	public:
		GameInstance();
		virtual ~GameInstance();

		void Init(bool loadStartScene = true);

		void OnSceneSimulationStart(Scene* scene);
		void OnSceneSimulationStop(Scene* scene);

		void OnUpdate(float ts);

		Scene* GetActiveScene();
		SceneManager* GetSceneManager();

		NetworkManager* GetNetworkManager();

		void SetNetMode(NetMode mode);
		NetMode GetNetMode() const;

		bool IsMainInstance() const { return m_IsMainInstance; }
		bool HasSimulationStarted() const { return m_SimulatedScenesCount > 0; }

	private:
		bool m_IsMainInstance = true;
		uint32_t m_SimulatedScenesCount = 0;
		uint32_t m_InstanceID = 0;

		Unique<SceneManager> m_SceneManager;
		Unique<NetworkManager> m_NetworkManager;
		ProjectSettings m_ProjectSettings;

	#ifdef PT_EDITOR
		SceneViewportPanel* m_EditorViewport;
	#endif

		friend class Application;
		friend class SceneManager;
		friend class Scene;
		friend class Client;
		friend class Server;

		friend class EditorLayer;
		friend class SceneViewportPanel;
		friend class SettingsPanel;
	};

}
