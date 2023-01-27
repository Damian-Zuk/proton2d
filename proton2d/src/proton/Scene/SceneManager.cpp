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
		for (auto& [scenePath, scene] : m_Scenes)
			delete scene;
	}

	void SceneManager::Init()
	{
		if (!s_Instance)
		{
			s_Instance = new SceneManager();
#if PROTON_EDITOR
			Scene* scene = CreateEmptyScene("<Unsaved scene>");
			s_Instance->m_ActiveScene = scene;
			EditorOverlay::SetSceneContext(scene);
#endif
		}
	}

	bool SceneManager::IsLoaded(const std::string& scenePath)
	{
		return s_Instance->m_Scenes.find(scenePath) != s_Instance->m_Scenes.end();
	}


	Scene* SceneManager::Load(const std::string& scenePath)
	{
		std::string filepath = "scenes/" + scenePath + ".scene";
		return s_Instance->Deserialize(scenePath, filepath);
	}


	Scene* SceneManager::LoadFromCache(const std::string& scenePath)
	{
		Scene* scene = new Scene(scenePath);
		std::string filepath = "cache/" + 
			(scenePath == "<Unsaved scene>" ? "unsaved_scene" : scenePath) + ".scene";
		std::replace(filepath.begin(), filepath.end(), '\\', '_');

		return s_Instance->Deserialize(scenePath, filepath);
	}


	Scene* SceneManager::Deserialize(const std::string& scenePath, const std::string& fullFilepath)
	{
		Scene* scene = CreateEmptyScene(scenePath);

		SceneSerializer serializer(scene);
		if (!serializer.Deserialize(fullFilepath))
		{
			LOG_ERROR("[SceneManager::Deserialize] Loading", fullFilepath, "failed!");
			return nullptr;
		}
		LOG_INFO("[SceneManager::Deserialize] Succesfuly loaded", fullFilepath);
		return scene;
	}


	void SceneManager::Unload(const std::string& scenePath)
	{
		delete s_Instance->m_Scenes.at(scenePath);
		s_Instance->m_Scenes.erase(scenePath);
	}


	Scene* SceneManager::SetActiveScene(const std::string& scenePath)
	{
		if (!IsLoaded(scenePath))
		{
			LOG_ERROR("[SceneManager::SetActiveScene] Error: scene not loaded!");
			return nullptr;
		}

#if PROTON_EDITOR
		// Unload unsaved scene if it's empty
		if (IsLoaded("<Unsaved scene>"))
		{
			Scene* scene = GetScene("<Unsaved scene>");
			if (!scene->m_Registry.size() && s_Instance->m_Scenes.size() > 1)
				Unload("<Unsaved scene>");
		}
#endif

		s_Instance->m_ActiveScene = s_Instance->m_Scenes.at(scenePath);

#if PROTON_EDITOR
		EditorOverlay::SetSceneContext(s_Instance->m_ActiveScene);
		EditorOverlay::SetInspectorContext({});
#endif

		Renderer::SetClearColor(s_Instance->m_ActiveScene->m_ClearColor);

		return s_Instance->m_ActiveScene;
	}


	void SceneManager::SaveSceneAs(const std::string& scenePath, const std::string& newScenePath)
	{
		if (!IsLoaded(scenePath))
		{
			LOG_ERROR("[SceneManager::SaveSceneAs] Error: scene", scenePath, "not loaded!");
			return;
		}

		SceneSerializer serializer(s_Instance->m_Scenes.at(scenePath));
		serializer.Serialize("scenes/" + newScenePath + ".scene");
	}


	void SceneManager::SaveActiveSceneAs(const std::string& scenePath)
	{
		SaveSceneAs(s_Instance->m_ActiveScene->m_SceneFilepath, scenePath);
	}


	void SceneManager::SaveActiveScene()
	{
		std::string filepath = s_Instance->m_ActiveScene->m_SceneFilepath;
		SaveSceneAs(filepath, filepath);
	}


	Scene* SceneManager::GetActiveScene()
	{
		return s_Instance->m_ActiveScene;
	}


	Scene* SceneManager::GetScene(const std::string& scenePath)
	{
		if (!IsLoaded(scenePath))
		{
			LOG_ERROR("[SceneManager::GetScene] Error: scene not found!");
			return nullptr;
		}
		return s_Instance->m_Scenes.at(scenePath);
	}


	const std::string& SceneManager::GetActiveSceneFilepath()
	{
		return s_Instance->m_ActiveScene->m_SceneFilepath;
	}


	Scene* SceneManager::CreateEmptyScene(const std::string& scenePath)
	{
		Scene* scene = new Scene();
		scene->m_SceneFilepath = scenePath;
		if (IsLoaded(scenePath))
			delete s_Instance->m_Scenes.at(scenePath);
		s_Instance->m_Scenes[scenePath] = scene;
		return scene;
	}

}
