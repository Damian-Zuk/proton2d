#pragma once

namespace proton {

	class Scene;

	/*
	* SceneManager class methods use scene filepaths - "scenePath" 
	* (realative to "scenes" directory) without ".scene.json" extension
	* as scene identifiers (keys in map storage)
	*/
	class SceneManager
	{
	public:
		Scene* GetScene(const std::string& scenePath);
		Scene* GetActiveScene();
		Scene* SetActiveScene(const std::string& scenePath);

		Scene* Load(const std::string& scenePath);
		void Unload(const std::string& scenePath);
		bool IsLoaded(const std::string& scenePath);

		void SaveSceneAs(const std::string& scenePath, const std::string& newSenePath);
		Scene* CreateEmptyScene(const std::string& scenePath = "<Unsaved scene>");

	private:
		Scene* m_ActiveScene = nullptr;
		std::map<std::string, Shared<Scene>> m_Scenes;

		friend class Application;
		friend class ToolbarPanel;

		friend class EditorLayer;
		friend class EditorMenuBar;
		friend class SceneViewportPanel;
	};
}
