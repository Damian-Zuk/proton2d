#include "pch.h"
#include "proton/Scene/SceneManager.h"
#include "proton/Scene/Scene.h"
#include "proton/Assets/SceneSerializer.h"

#if PROTON_EDITOR
#include "proton/Editor/EditorOverlay.h"
#endif

namespace proton {

	SceneManager* SceneManager::s_Instance = nullptr;

	SceneManager::SceneManager()
	{
#if PROTON_EDITOR
		Scene* scene = new Scene();
		scene->m_SceneState = SceneState::Edit;
		m_Scenes["EDITOR_EMPTY_SCENE"] = scene;
		m_ActiveScene = scene;
		m_ActiveSceneName = "EDITOR_EMPTY_SCENE";
		EditorOverlay::SetSceneContext(scene);
#endif
	}

	SceneManager::~SceneManager()
	{
		for (auto& [sceneName, scene] : m_Scenes)
			delete scene;
	}

	void SceneManager::Load(const std::string& sceneName)
	{
		Scene* scene = new Scene(sceneName);

#if PROTON_EDITOR
		scene->m_SceneState = SceneState::Edit;
#endif
		SceneSerializer serializer(scene);
		serializer.Deserialize("scenes/" + sceneName + ".json");
		scene->m_SceneFilepath = sceneName + ".json";

		if (s_Instance->m_Scenes.find(sceneName) != s_Instance->m_Scenes.end())
			delete s_Instance->m_Scenes[sceneName];

		s_Instance->m_Scenes[sceneName] = scene;
	}

	void SceneManager::UnLoad(const std::string& sceneName)
	{
		delete s_Instance->m_Scenes.at(sceneName);
		s_Instance->m_Scenes.erase(sceneName);
	}

	void SceneManager::SetActiveScene(const std::string& sceneName)
	{
		if (s_Instance->m_ActiveScene && s_Instance->m_ActiveScene->GetSceneState() == SceneState::Play)
			s_Instance->m_ActiveScene->OnEndPlay();

		s_Instance->m_ActiveScene = s_Instance->m_Scenes.at(sceneName);
		s_Instance->m_ActiveSceneName = sceneName;

#if PROTON_EDITOR
		EditorOverlay::SetSceneContext(s_Instance->m_ActiveScene);
#else
		s_Instance->m_ActiveScene->OnBeginPlay();
#endif
	}

	Scene* SceneManager::GetActiveScene()
	{
		return s_Instance->m_ActiveScene;
	}

}
