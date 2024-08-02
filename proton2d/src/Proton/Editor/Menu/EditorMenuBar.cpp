#include "ptpch.h"
#ifdef PT_EDITOR
#include "Proton/Editor/Menu/EditorMenuBar.h"
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
		static bool openNewInstancePopupModal = false;

		if (ImGui::BeginMenuBar())
		{
			if (ImGui::BeginMenu("File"))
			{
				if (ImGui::MenuItem("Open scene...", "Ctrl+O"))
					OpenScene();

				ImGui::Separator();

				if (ImGui::MenuItem("New scene", "Ctrl+N"))
					NewScene();

				if (ImGui::MenuItem("Save scene", "Ctrl+S"))
					SaveScene();

				if (ImGui::MenuItem("Save scene as...", "Ctrl+Shift+S"))
					SaveSceneAs();

				ImGui::Separator();

				if (ImGui::MenuItem("Exit"))
					Application::Get().Exit();

				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Project"))
			{
				if (ImGui::MenuItem("Project proporties"))
					m_OpenProjectProporties = true;
				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Editor"))
			{
				if (ImGui::MenuItem("Reset camera"))
				{
					auto viewport = EditorLayer::GetFocusedViewportPanel();
					viewport->m_Camera->SetPosition({0.0f, 0.0f, 0.0f});
				}
				ImGui::EndMenu();
			}

			ImGui::EndMenuBar();
		}

		HandleProjectProportiesPopup();
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
		Scene* scene = manager->GetActiveScene();
		if (!scene)
			return;

		if (scene->m_SceneFilepath != "<Unsaved scene>")
		{
			const std::string filepath = scene->m_SceneFilepath;
			manager->SaveSceneAs(filepath, filepath);
		}
		else
			SaveSceneAs();
	}

	void EditorMenuBar::SaveSceneAs()
	{
		SceneManager* manager = Application::GetGameInstance()->GetSceneManager();
		Scene* scene = manager->GetActiveScene();
		if (!scene)
			return;

		std::string filepath = GetSceneFilename(FileDialogs::SaveFile(".scene.json"));
		if (filepath.size())
		{
			manager->SaveSceneAs(scene->m_SceneFilepath, filepath);
			if (scene->m_SceneFilepath == "<Unsaved scene>")
			{
				manager->Unload("<Unsaved scene>");
			}
			manager->Load(filepath);
			manager->SetActiveScene(filepath);
		}
	}

	void EditorMenuBar::HandleProjectProportiesPopup()
	{
		if (m_OpenProjectProporties)
			ImGui::OpenPopup("Project Proporties");

		if (ImGui::BeginPopupModal("Project Proporties", NULL, ImGuiWindowFlags_AlwaysAutoResize))
		{
			ProjectSettings& project = Application::Get().GetGameInstance()->m_ProjectSettings;

			char buffer[256];
			strcpy_s(buffer, project.m_StartScene.c_str());

			ImGui::Dummy({ 0, 5 });
			ImGui::PushItemWidth(150.0f);
			if (ImGui::InputText("Startup scene", buffer, 256))
			{
				project.m_StartScene = buffer;
			}
			ImGui::PopItemWidth();
			ImGui::Dummy({ 0, 5 });

			if (ImGui::Button("Save", {150, 0}))
			{
				project.WriteProjectSettings();
				ImGui::CloseCurrentPopup();
			}
			ImGui::SetItemDefaultFocus();
			ImGui::SameLine();
			if (ImGui::Button("Cancel", { 150, 0 })) { ImGui::CloseCurrentPopup(); }

			ImGui::EndPopup();
			m_OpenProjectProporties = false;
		}
	}

}

#endif // PT_EDITOR
