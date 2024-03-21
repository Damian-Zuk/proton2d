#include "ptpch.h"
#include "Proton/Scene/SceneManager.h"
#include "Proton/Scene/Scene.h"
#include "Proton/Assets/SceneSerializer.h"
#include "Proton/Core/Application.h"
#include "Proton/Core/GameInstance.h"
#include "Proton/Graphics/Renderer/Renderer.h"

#ifdef PT_EDITOR
#include "Proton/Editor/EditorLayer.h"
#include "Proton/Editor/Panels/SceneViewportPanel.h"
#endif

namespace proton {

	SceneManager::SceneManager(GameInstance* gameInstance)
		: m_GameInstance(gameInstance)
	{
	}

	Scene* SceneManager::GetScene(const std::string& scenePath)
	{
		return m_Scenes.at(scenePath).get();
	}

	Scene* SceneManager::GetActiveScene()
	{
		return m_ActiveScene;
	}

	Scene* SceneManager::SetActiveScene(const std::string& scenePath)
	{
		if (!IsLoaded(scenePath))
		{
			if (!Load(scenePath))
			{
				PT_CORE_ERROR("Scene '{}' not loaded!", scenePath);
				return nullptr;
			}
		}

		PT_CORE_INFO("scene='{}'", scenePath);
		SetActiveScene(GetScene(scenePath));
		Renderer::SetClearColor(m_ActiveScene->m_ClearColor);

		return m_ActiveScene;
	}

	Scene* SceneManager::SetActiveScene(Scene* scene)
	{
		m_ActiveScene = scene;
	#ifdef PT_EDITOR
		if (!m_GameInstance->IsMainInstance())
			m_GameInstance->m_EditorViewport->m_ActiveScene = scene;
		else
			EditorLayer::SetActiveScene(scene);
	#endif
		return scene;
	}

	Scene* SceneManager::Load(const std::string& scenePath)
	{
		PT_CORE_INFO("file='{}.scene.json'", scenePath);
		Shared<Scene> scene = MakeShared<Scene>(std::string(), scenePath);
		SceneSerializer serializer(scene.get());
		scene->m_GameInstance = m_GameInstance;

		std::string filepath = "content/scenes/" + scenePath + ".scene.json";
		if (!serializer.Deserialize(filepath))
		{
			PT_CORE_ERROR("Loading '{}' failed!", filepath);
			return nullptr;
		}
		m_Scenes[scenePath] = scene;
		return scene.get();
	}

	void SceneManager::Unload(const std::string& scenePath)
	{
		Scene* scene = GetScene(scenePath);
		if (!scene)
		{
			PT_CORE_ERROR("scene='{}' not found", scenePath);
			return;
		}

		PT_CORE_INFO("scene='{}'", scenePath);
		bool active = scene == m_ActiveScene;
		m_Scenes.erase(scenePath);

		if (active)
		{
			if (m_Scenes.size())
				SetActiveScene(m_Scenes.begin()->first);
			else
				SetActiveScene(nullptr);
		}
	}

	bool SceneManager::IsLoaded(const std::string& scenePath)
	{
		return m_Scenes.find(scenePath) != m_Scenes.end();
	}

	void SceneManager::SaveSceneAs(const std::string& scenePath, const std::string& newScenePath)
	{
		if (!IsLoaded(scenePath))
		{
			PT_CORE_ERROR("Scene '{}' not loaded!", scenePath);
			return;
		}

		SceneSerializer serializer(GetScene(scenePath));
		serializer.Serialize("content/scenes/" + newScenePath + ".scene.json");
	}

	Scene* SceneManager::CreateEmptyScene(const std::string& scenePath)
	{
		Shared<Scene> scene = MakeShared<Scene>("Unnamed Scene", "<Unsaved scene>");
		scene->m_GameInstance = m_GameInstance;
		m_Scenes["<Unsaved scene>"] = scene;
		return scene.get();
	}

}
