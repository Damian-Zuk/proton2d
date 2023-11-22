#include "pch.h"
#include "proton/Editor/Panels/ScenePanel.h"
#include "proton/Editor/EditorLayer.h"
#include "proton/Scene/SceneManager.h"
#include "proton/Utils/Utils.h"

#include <imgui.h>


namespace proton {

	void ScenePanel::OnImGuiRender()
	{
		ImGui::Begin("Scene");
		ImGui::Dummy({ 0, 1.0f });

		ImGui::SetCursorPosX((ImGui::GetWindowWidth() 
			- (m_ActiveScene->m_SceneState == SceneState::Stop ? 75 : 160)) / 2.0f);

		// Play / stop / resume buttons
		if (m_ActiveScene->m_SceneState != SceneState::Stop)
		{
			if (ImGui::Button("Stop", { 75, 30 }))
				StopSceneSimulation();

			ImGui::SameLine();
			if (m_ActiveScene->m_SceneState == SceneState::Paused)
			{
				if (ImGui::Button("Resume", { 75, 30 }))
					m_ActiveScene->Pause(false);
			}
			else
			{
				if (ImGui::Button("Pause", { 75, 30 }))
					m_ActiveScene->Pause(true);
			}
		}
		else
		{
			if (ImGui::Button("Play", { 75, 30 }))
				m_ActiveScene->BeginPlay();
		}

		// Scene name text
		ImGui::Dummy({ 0, 5 }); ImGui::Separator(); ImGui::Dummy({ 0, 1 });
		std::string sceneText = m_ActiveScene->m_SceneFilepath;
		if (m_SavedSceneTextTimer > 0.0f)
			sceneText = "(Save Success) " + sceneText;
		ImGui::SetCursorPosX((ImGui::GetWindowWidth() - ImGui::CalcTextSize(sceneText.c_str()).x) / 2);
		ImGui::Text(sceneText.c_str());
		ImGui::Dummy({ 0, 3 });

		// Center buttons
		ImGui::SetCursorPosX(ImGui::GetWindowWidth() / 2 - 105);

		// New scene button
		if (ImGui::Button("New scene", { 100, 25 }))
		{
			ImGui::OpenPopup("save_current_scene");
		}
		// New scene popup
		if (ImGui::BeginPopup("save_current_scene"))
		{
			ImGui::Text("Save current scene?");
			if (ImGui::Button("Yes"))
			{
				if (m_ActiveScene->m_SceneFilepath == "<Unsaved scene>")
					SaveSceneAs();
				else
					SceneManager::SaveActiveScene();
				ImGui::CloseCurrentPopup();
				CreateNewScene();
			}
			ImGui::SameLine();
			if (ImGui::Button("No"))
			{
				ImGui::CloseCurrentPopup();
				CreateNewScene();
			}
			ImGui::SameLine();
			if (ImGui::Button("Cancel"))
			{
				ImGui::CloseCurrentPopup();
			}
			ImGui::EndPopup();
		}

		// Open scene button
		ImGui::SameLine();
		if (ImGui::Button("Open scene", { 100, 25 }))
			OpenScene();

		ImGui::Dummy({ 0, 5.0f });
		ImGui::SetCursorPosX(ImGui::GetWindowWidth() / 2 - 105);

		// Save scene button
		if (ImGui::Button("Save", { 100, 25 }))
			SaveScene();

		// Save us button
		ImGui::SameLine();
		if (ImGui::Button("Save as", { 100, 25 }))
			SaveSceneAs();

		ImGui::Dummy({ 0, 5 });
		ImGui::Separator();
		ImGui::Dummy({ 0, 2 });

		// Scenes loaded in memory view
		DrawSceneMemoryView();

		ImGui::End();
	}

	void ScenePanel::OnUpdate(float ts) 
	{
		if (m_SavedSceneTextTimer == 2.0f)
			m_SavedSceneTextTimer = 1.999f;
		else
			m_SavedSceneTextTimer = glm::max(m_SavedSceneTextTimer - ts, 0.0f);
	}

	static std::string GetSceneFilename(const std::string& filepath)
	{
		std::size_t pos = filepath.find("scenes");
		if (pos != std::string::npos) {
			std::string filename = filepath.substr(pos + 7);
			std::size_t posExt = filepath.find(".scene");
			if (posExt != std::string::npos)
				return filename.substr(0, filename.size() - 6);
			return filename;
		}
		return std::string();
	}

	void ScenePanel::CreateNewScene()
	{
		Scene* scene = SceneManager::CreateEmptyScene("<Unsaved scene>");
		SceneManager::s_Instance->m_ActiveScene = scene;
		EditorLayer::SetActiveScene(scene);
	}

	void ScenePanel::OpenScene()
	{
		std::string sceneFile = GetSceneFilename(FileDialogs::OpenFile("scene"));
		if (sceneFile.size())
		{
			SceneManager::Load(sceneFile);
			SceneManager::SetActiveScene(sceneFile);
		}
	}

	void ScenePanel::SaveScene()
	{
		if (m_ActiveScene->m_SceneFilepath != "<Unsaved scene>")
		{
			SceneManager::SaveActiveScene();
			m_SavedSceneTextTimer = 2.0f;
		}
		else
			SaveSceneAs();
	}

	void ScenePanel::SaveSceneAs()
	{ 
		std::string filepath = GetSceneFilename(FileDialogs::SaveFile(".scene"));
		if (filepath.size())
		{
			SceneManager::SaveActiveSceneAs(filepath);
			if (m_ActiveScene->m_SceneFilepath == "<Unsaved scene>")
			{
				SceneManager::Unload("<Unsaved scene>");
			}
			SceneManager::Load(filepath);
			SceneManager::SetActiveScene(filepath);
			m_SavedSceneTextTimer = 2.0f;
		}
	}

	void ScenePanel::StopSceneSimulation()
	{
		std::string filepath = m_ActiveScene->GetFilepath();
		if (filepath.size())
		{
			SceneManager::LoadFromCache(filepath);
			SceneManager::SetActiveScene(filepath);
		}
	}

	void ScenePanel::DrawSceneMemoryView()
	{
		if (ImGui::TreeNodeEx("Memory view", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::Dummy({ 0, 2 });
			for (auto& [scenePath, scenePtr] : SceneManager::s_Instance->m_Scenes)
			{
				ImGui::Text(scenePath.c_str());
				bool isActive = m_ActiveScene == scenePtr;

				ImGui::SameLine(ImGui::GetWindowWidth() - (isActive ? 75 : 160));
				if (!isActive && ImGui::Button(("Set active##" + scenePath).c_str()))
					SceneManager::SetActiveScene(scenePath);

				if (isActive)
				{
					ImGui::PushStyleColor(ImGuiCol_Text, { 0.0f, 1.0f, 0.2f, 1.0f });
					ImGui::Text("Active");
					ImGui::PopStyleColor();
					ImGui::Dummy({ 0, 2 });
				}
				else
				{
					ImGui::SameLine();
					if (ImGui::Button(("Unload##" + scenePath).c_str()))
					{
						SceneManager::Unload(scenePath);
						break;
					}
				}
			}
			ImGui::TreePop();
		}
	}

}
