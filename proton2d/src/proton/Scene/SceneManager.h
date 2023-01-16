#pragma once

namespace proton {

	class Scene;

	class SceneManager
	{
	public:
		SceneManager() = default;
		~SceneManager();

		static void Init();

		static Scene* CreateNewEmptyScene(const std::string& sceneName);
		static void Load(const std::string& sceneName);
		static void Unload(const std::string& sceneName);
		static void SetActiveScene(const std::string& sceneName);
		static Scene* GetActiveScene();
		static const std::string& GetActiveSceneFilepath();

	private:
		static SceneManager* s_Instance;

		Scene* m_ActiveScene = nullptr;
		std::unordered_map<std::string, Scene*> m_Scenes;

		friend class Application;
		friend class EditorOverlay;
	};
}