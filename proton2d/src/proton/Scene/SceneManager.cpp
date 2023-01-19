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
		scene->m_SceneState = SceneState::Paused;
#endif
		s_Instance->m_Scenes[sceneName] = scene;
		return scene;
	}

	Scene* SceneManager::Load(const std::string& sceneName)
	{
		Scene* scene = new Scene(sceneName);

#if PROTON_EDITOR
		scene->m_SceneState = SceneState::Edit;
#else
		scene->m_SceneState = SceneState::Paused;
#endif
		SceneSerializer serializer(scene);
		if (!serializer.Deserialize("scenes/" + sceneName + ".scene"))
		{
			LOG_ERROR("[SceneManager::Load] File not found ", sceneName + ".scene");
			return nullptr;
		}

		scene->m_SceneFilepath = sceneName;
		if (IsLoaded(sceneName))
			delete s_Instance->m_Scenes[sceneName];

		s_Instance->m_Scenes[sceneName] = scene;
		LOG_INFO("[SceneManager::Load] Loaded", sceneName)
		return scene;
}

	void SceneManager::Unload(const std::string& sceneName)
	{
		delete s_Instance->m_Scenes.at(sceneName);
		s_Instance->m_Scenes.erase(sceneName);
	}

	bool SceneManager::IsLoaded(const std::string& sceneName)
	{
		return s_Instance->m_Scenes.find(sceneName) != s_Instance->m_Scenes.end();
	}

	void SceneManager::SaveSceneAs(const std::string& sceneName, const std::string& filepath)
	{
		if (!IsLoaded(sceneName))
		{
			LOG_ERROR("[SceneManager::SaveSceneAs] Error: scene not loaded!");
			return;
		}

		SceneSerializer serializer(s_Instance->m_Scenes.at(sceneName));
		serializer.Serialize("scenes/" + filepath + ".scene");
	}

	void SceneManager::SaveActiveSceneAs(const std::string& filepath)
	{
		SaveSceneAs(s_Instance->m_ActiveScene->m_SceneFilepath, filepath);
	}

	Scene* SceneManager::SetActiveScene(const std::string& sceneName, bool endCurrentScene)
	{
		if (!IsLoaded(sceneName))
		{
			LOG_ERROR("[SceneManager::SetActiveScene] Error: scene not loaded!");
			return nullptr;
		}

		if (endCurrentScene && s_Instance->m_ActiveScene &&
			s_Instance->m_ActiveScene->GetSceneState() == SceneState::Play)
			s_Instance->m_ActiveScene->EndPlay();
		else if (s_Instance->m_ActiveScene)
			s_Instance->m_ActiveScene->m_SkipUpdate = true;

		s_Instance->m_ActiveScene = s_Instance->m_Scenes.at(sceneName);

#if PROTON_EDITOR
		EditorOverlay::SetSceneContext(s_Instance->m_ActiveScene);
		EditorOverlay::SetInspectorContext(Entity{});
#endif

		Renderer::SetClearColor(s_Instance->m_ActiveScene->m_ClearColor);

		return s_Instance->m_ActiveScene;
	}

	Scene* SceneManager::GetActiveScene()
	{
		return s_Instance->m_ActiveScene;
	}

	Scene* SceneManager::GetScene(const std::string& sceneName)
	{
		if (IsLoaded(sceneName))
			return nullptr;
		return s_Instance->m_Scenes.at(sceneName);
	}

	const std::string& SceneManager::GetActiveSceneFilepath()
	{
		return s_Instance->m_ActiveScene->m_SceneFilepath;
	}

}
