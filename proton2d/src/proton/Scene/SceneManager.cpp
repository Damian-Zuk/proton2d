#include "pch.h"
#include "proton/Scene/SceneManager.h"
#include "proton/Scene/Scene.h"
#include "proton/Assets/SceneSerializer.h"
#include "proton/Core/Application.h"
#include "proton/Graphics/Renderer.h"

#if PROTON_EDITOR
#include "proton/Editor/EditorOverlay.h"
#endif

namespace proton {

	SceneManager* SceneManager::s_Instance = nullptr;

	SceneManager::~SceneManager()
	{
		for (auto& [sceneName, scene] : m_Scenes)
			delete scene;
	}

	void SceneManager::Init()
	{
		if (!s_Instance)
		{
			s_Instance = new SceneManager();
#if PROTON_EDITOR
			Scene* scene = CreateNewEmptyScene("<Unsaved scene>");
			s_Instance->m_ActiveScene = scene;
			EditorOverlay::SetSceneContext(scene);
#endif
		}
	}

	Scene* SceneManager::CreateNewEmptyScene(const std::string& sceneName)
	{
		Scene* scene = new Scene();
#if PROTON_EDITOR
		scene->m_SceneState = SceneState::Edit;
#else
		scene->m_SceneState = SceneState::Play;
#endif
		s_Instance->m_Scenes[sceneName] = scene;
		return scene;
	}

	void SceneManager::Load(const std::string& sceneName)
	{
		Scene* scene = new Scene(sceneName);

#if PROTON_EDITOR
		scene->m_SceneState = SceneState::Edit;
#else
		scene->m_SceneState = SceneState::Play;
#endif
		SceneSerializer serializer(scene);
		if (!serializer.Deserialize("scenes/" + sceneName))
		{
			LOG_ERROR("[SceneManager] File not found ", sceneName);
			return;
		}

		scene->m_SceneFilepath = sceneName;
		if (s_Instance->m_Scenes.find(sceneName) != s_Instance->m_Scenes.end())
			delete s_Instance->m_Scenes[sceneName];

		s_Instance->m_Scenes[sceneName] = scene;
		LOG_INFO("[SceneManager] Loaded", sceneName)
}

	void SceneManager::Unload(const std::string& sceneName)
	{
		delete s_Instance->m_Scenes.at(sceneName);
		s_Instance->m_Scenes.erase(sceneName);
	}

	void SceneManager::SetActiveScene(const std::string& sceneName)
	{
		if (s_Instance->m_ActiveScene && s_Instance->m_ActiveScene->GetSceneState() == SceneState::Play)
			s_Instance->m_ActiveScene->OnEndPlay();

		s_Instance->m_ActiveScene = s_Instance->m_Scenes.at(sceneName);

#if PROTON_EDITOR
		EditorOverlay::SetSceneContext(s_Instance->m_ActiveScene);
#else
		s_Instance->m_ActiveScene->OnBeginPlay();
#endif
		Renderer::SetClearColor(s_Instance->m_ActiveScene->m_ClearColor);
	}

	Scene* SceneManager::GetActiveScene()
	{
		return s_Instance->m_ActiveScene;
	}

	const std::string& SceneManager::GetActiveSceneFilepath()
	{
		return s_Instance->m_ActiveScene->m_SceneFilepath;
	}

}
