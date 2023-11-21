#pragma once

namespace proton {

	class Scene;

	/*
	* SceneManager class methods use scene filepaths - "scenePath" 
	* (realative to "scenes" directory) without ".scene" extension
	* as scene identifiers (keys in map storage)
	*/
	class SceneManager
	{
	public:
		SceneManager() = default;
		~SceneManager();

		static bool IsLoaded(const std::string& scenePath);

		static Scene* Load(const std::string& scenePath);
		static void Unload(const std::string& scenePath);

		static Scene* SetActiveScene(const std::string& scenePath);
		static Scene* GetActiveScene();

		static Scene* GetScene(const std::string& scenePath);

		static void SaveSceneAs(const std::string& scenePath, const std::string& mewSenePath);
		static void SaveActiveSceneAs(const std::string& scenePath);
		static void SaveActiveScene();

		static const std::string& GetActiveSceneFilepath();

	private:
		static void Init();
		static Scene* LoadFromCache(const std::string& scenePath);
		static Scene* CreateEmptyScene(const std::string& scenePath);

		Scene* Deserialize(const std::string& scenePath, const std::string& fullFilepath);
	private:
		static SceneManager* s_Instance;

		Scene* m_ActiveScene = nullptr;
		std::map<std::string, Scene*> m_Scenes;

		friend class Application;
		friend class ScenePanel;
	};
}