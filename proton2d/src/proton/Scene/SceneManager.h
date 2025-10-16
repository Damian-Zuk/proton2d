#pragma once

namespace proton {

	class Scene;
	class GameInstance;

	class SceneManager
	{
	public:
		SceneManager(GameInstance* gameInstance);
		virtual ~SceneManager() = default;

		void OnUpdate(float ts);

		Scene* GetActiveScene();
		Scene* SetActiveScene(const std::string& scenePath);
		Scene* SetActiveScene(Scene* scene);
		Scene* GetScene(const std::string& scenePath);

		Scene* Load(const std::string& scenePath);
		void Unload(const std::string& scenePath);
		bool IsLoaded(const std::string& scenePath);

		void SaveSceneAs(const std::string& scenePath, const std::string& newSenePath);
		Scene* CreateEmptyScene(const std::string& scenePath = "<Unsaved scene>");

	private:
		GameInstance* m_GameInstance;

		Scene* m_ActiveScene = nullptr;
		std::map<std::string, Shared<Scene>> m_Scenes;

		friend class Application;
		friend class NetReplicator;

		friend class EditorLayer;
		friend class EditorMenuBar;
		friend class ToolbarPanel;
		friend class SceneViewportPanel;
	};
}
