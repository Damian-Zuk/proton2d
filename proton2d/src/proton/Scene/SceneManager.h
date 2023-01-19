#pragma once

namespace proton {

	class Scene;

	class SceneManager
	{
	public:
		SceneManager() = default;
		~SceneManager();

		static Scene* CreateNewEmptyScene(const std::string& sceneName);
		static Scene* Load(const std::string& sceneName);
		static Scene* SetActiveScene(const std::string& sceneName, bool endCurrentScene = false);
		static Scene* GetActiveScene();
		static Scene* GetScene(const std::string& sceneName);
		static void Unload(const std::string& sceneName);
		static bool IsLoaded(const std::string& sceneName);
		// filepath without .scene extension
		static void SaveSceneAs(const std::string& sceneName, const std::string& filepath);
		static void SaveActiveSceneAs(const std::string& filepath);
		static const std::string& GetActiveSceneFilepath();

	private:
		static void Init();
		static SceneManager* s_Instance;

		Scene* m_ActiveScene = nullptr;
		std::unordered_map<std::string, Scene*> m_Scenes;

		friend class Application;
		friend class EditorOverlay;
	};
}