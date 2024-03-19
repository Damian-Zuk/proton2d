#pragma once
#include "Proton/Scene/SceneManager.h"
#include "Proton/Networking/NetworkManager.h"

namespace proton {

	class GameInstance
	{
	public:
		GameInstance();
		virtual ~GameInstance();

		SceneManager* GetSceneManager() { return &m_SceneManager; }

		void SetNetMode(NetMode mode) { return m_NetworkManager.SetNetMode(mode); }
		NetMode GetNetMode() { return m_NetworkManager.GetNetMode(); }

	private:
		SceneManager m_SceneManager;
		NetworkManager m_NetworkManager;

		friend class Application;
		friend class SceneViewportPanel;
	};

}
