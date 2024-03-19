#include "ptpch.h"
#include "Proton/Scene/SceneManager.h"
#include "Proton/Scene/Scene.h"
#include "Proton/Assets/SceneSerializer.h"
#include "Proton/Core/Application.h"
#include "Proton/Graphics/Renderer/Renderer.h"

#ifdef PT_EDITOR
#include "Proton/Editor/EditorLayer.h"
#endif

namespace proton {

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
		if (!IsLoaded(scenePath) && !Load(scenePath))
		{
			PT_CORE_ERROR("Scene '{}' not loaded!", scenePath);
			return nullptr;
		}

		PT_CORE_INFO("scene='{}'", scenePath);
		Scene* targetScene = GetScene(scenePath);
		m_ActiveScene = targetScene;
		Renderer::SetClearColor(m_ActiveScene->m_ClearColor);

	#ifdef PT_EDITOR
		EditorLayer::SetActiveScene(targetScene);
	#endif
		return m_ActiveScene;
	}

	Scene* SceneManager::Load(const std::string& scenePath)
	{
		PT_CORE_INFO("file='{}.scene.json'", scenePath);
		Shared<Scene> scene = MakeShared<Scene>(std::string(), scenePath);
		SceneSerializer serializer(scene.get());

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
		bool isActive = scene == m_ActiveScene;
		m_Scenes.erase(scenePath);

		if (isActive)
		{
			if (m_Scenes.size())
			{
				// If unloaded active scene, switch to first scene
				SetActiveScene(m_Scenes.begin()->first);
			}
			else
			{
				m_ActiveScene = nullptr;
			#ifdef PT_EDITOR
				EditorLayer::SetActiveScene(nullptr);
			#endif
			}
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
		m_Scenes["<Unsaved scene>"] = scene;
		return scene.get();
	}

}
