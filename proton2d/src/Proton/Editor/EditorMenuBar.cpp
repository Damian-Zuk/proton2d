#include "ptpch.h"
#ifdef PT_EDITOR
#include "Proton/Editor/EditorMenuBar.h"
#include "Proton/Editor/EditorLayer.h"
#include "Proton/Editor/Panels/SceneViewportPanel.h"
#include "Proton/Core/Application.h"
#include "Proton/Core/GameInstance.h"
#include "Proton/Scene/SceneManager.h"
#include "Proton/Utils/Utils.h"

#include <imgui.h>

namespace proton {

	void EditorMenuBar::OnImGuiRender()
	{
		if (ImGui::BeginMenuBar())
		{
			if (ImGui::BeginMenu("File"))
			{
				if (ImGui::MenuItem("Open Scene...", "Ctrl+O"))
					OpenScene();

				ImGui::Separator();

				if (ImGui::MenuItem("New Scene", "Ctrl+N"))
					NewScene();

				if (ImGui::MenuItem("Save Scene", "Ctrl+S"))
					SaveScene();

				if (ImGui::MenuItem("Save Scene As...", "Ctrl+Shift+S"))
					SaveSceneAs();

				ImGui::Separator();

				if (ImGui::MenuItem("Exit"))
					Application::Get().Exit();

				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Editor"))
			{
				if (ImGui::MenuItem("Reset Camera"))
					EditorLayer::GetCamera()->SetPosition({0.0f, 0.0f, 0.0f});
				ImGui::EndMenu();
			}

			ImGui::EndMenuBar();
		}
	}

	static std::string GetSceneFilename(const std::string& filepath)
	{
		std::size_t pos = filepath.find("scenes");
		if (pos != std::string::npos) {
			std::string filename = filepath.substr(pos + 7);
			std::size_t posExt = filepath.find(".scene.json");
			if (posExt != std::string::npos)
				return filename.substr(0, filename.size() - 11);
			return filename;
		}
		return std::string();
	}

	void EditorMenuBar::NewScene()
	{
		SceneManager* manager = Application::GetGameInstance()->GetSceneManager();
		Scene* scene = manager->CreateEmptyScene();
		manager->m_ActiveScene = scene;
		EditorLayer::SetActiveScene(scene);
	}

	void EditorMenuBar::OpenScene()
	{
		SceneManager* manager = Application::GetGameInstance()->GetSceneManager();
		std::string sceneFile = GetSceneFilename(FileDialogs::OpenFile("scene"));
		if (sceneFile.size())
		{
			manager->Load(sceneFile);
			manager->SetActiveScene(sceneFile);
		}
	}

	void EditorMenuBar::SaveScene()
	{
		SceneManager* manager = Application::GetGameInstance()->GetSceneManager();
		Scene* activeScene = manager->GetActiveScene();
		if (!activeScene)
			return;

		if (activeScene->m_SceneFilepath != "<Unsaved scene>")
		{
			const std::string filepath = activeScene->m_SceneFilepath;
			manager->SaveSceneAs(filepath, filepath);
		}
		else
			SaveSceneAs();
	}

	void EditorMenuBar::SaveSceneAs()
	{
		SceneManager* manager = Application::GetGameInstance()->GetSceneManager();
		Scene* activeScene = manager->GetActiveScene();
		if (!activeScene)
			return;

		std::string filepath = GetSceneFilename(FileDialogs::SaveFile(".scene.json"));
		if (filepath.size())
		{
			manager->SaveSceneAs(activeScene->m_SceneFilepath, filepath);
			if (activeScene->m_SceneFilepath == "<Unsaved scene>")
			{
				manager->Unload("<Unsaved scene>");
			}
			manager->Load(filepath);
			manager->SetActiveScene(filepath);
		}
	}

}

#endif // PT_EDITOR
