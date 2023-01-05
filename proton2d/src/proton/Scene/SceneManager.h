#pragma once

namespace proton {

	class Scene;

	class SceneManager
	{
	public:
		SceneManager();
		~SceneManager();

		static void Load(const std::string& sceneName);
		static void UnLoad(const std::string& sceneName);
		static void SetActiveScene(const std::string& sceneName);
		static Scene* GetActiveScene();

	private:
		static SceneManager* s_Instance;

		Scene* m_ActiveScene = nullptr;
		std::string m_ActiveSceneName;
		std::unordered_map<std::string, Scene*> m_Scenes;

		friend class Application;
		friend class EditorOverlay;
	};
}