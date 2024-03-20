#pragma once
#include "Proton/Core/Project.h"
#include "Proton/Scene/SceneManager.h"
#include "Proton/Network/Common/NetworkManager.h"

namespace proton {

	class SceneViewportPanel;

	class GameInstance
	{
	public:
		GameInstance();
		virtual ~GameInstance();

		void Init();

		Scene* GetActiveScene() { return m_SceneManager.GetActiveScene(); }
		SceneManager* GetSceneManager() { return &m_SceneManager; }

		void SetNetMode(NetMode mode) { return m_NetworkManager.SetNetMode(mode); }
		NetMode GetNetMode() const { return m_NetworkManager.GetNetMode(); }

		bool IsMainInstance() const { return m_IsMainInstance; }

	private:
		SceneManager m_SceneManager;
		NetworkManager m_NetworkManager;

		Project m_Project;
		bool m_IsMainInstance = true;

	#ifdef PT_EDITOR
		SceneViewportPanel* m_EditorViewport;
	#endif

		friend class Application;
		friend class SceneManager;

		friend class EditorLayer;
		friend class SceneViewportPanel;
		friend class SettingsPanel;
	};

}
