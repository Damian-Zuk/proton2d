#include "pch.h"
#include "proton/Editor/Panels/ScenePanel.h"
#include "proton/Editor/EditorLayer.h"
#include "proton/Scene/SceneManager.h"
#include "proton/Utils/Utils.h"

#include <imgui.h>


namespace proton {

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

	void ScenePanel::OnImGuiRender()
	{
		ImGui::Begin("Scene");
		ImGui::Dummy({ 0, 1.0f });

		SceneState sceneState = m_ActiveScene->m_SceneState;
		ImGui::SetCursorPosX((ImGui::GetWindowWidth() - (sceneState == SceneState::Stop ? 75 : 160)) / 2.0f);

		// Play / stop / resume buttons
		bool stopSimulaton = false;
		if (sceneState != SceneState::Stop)
		{
			if (ImGui::Button("Stop", { 75, 30 }))
				stopSimulaton = true;

			ImGui::SameLine();
			if (m_ActiveScene->GetSceneState() == SceneState::Paused)
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

		// Stop button logic
		if (stopSimulaton)
		{
			std::string filepath = m_ActiveScene->GetFilepath();
			if (filepath.size())
			{
				SceneManager::LoadFromCache(filepath);
				SceneManager::SetActiveScene(filepath);
			}
		}

		// Scene name text
		ImGui::Dummy({ 0, 5 }); ImGui::Separator(); ImGui::Dummy({ 0, 1 });
		std::string sceneText = "Scene: " + m_ActiveScene->m_SceneFilepath;
		if (m_SavedSceneTextTimer > 0.0f)
			sceneText = "(Save success) " + sceneText;
		ImGui::SetCursorPosX((ImGui::GetWindowWidth() - ImGui::CalcTextSize(sceneText.c_str()).x) / 2);
		ImGui::Text(sceneText.c_str());
		ImGui::Dummy({ 0, 3 });

		// Center buttons
		ImGui::SetCursorPosX(ImGui::GetWindowWidth() / 2 - 105);

		bool saveAs = false;
		bool createNewScene = false;

		// New scene button
		if (ImGui::Button("New scene", { 100, 25 }))
		{
			ImGui::OpenPopup("save_current_scene");
		}
		if (ImGui::BeginPopup("save_current_scene"))
		{
			ImGui::Text("Save current scene?");
			if (ImGui::Button("Yes"))
			{
				createNewScene = true;
				if (m_ActiveScene->m_SceneFilepath != "<Unsaved scene>")
					SceneManager::SaveActiveScene();
				else
					saveAs = true;
				ImGui::CloseCurrentPopup();
			}
			ImGui::SameLine();
			if (ImGui::Button("No"))
			{
				createNewScene = true;
				ImGui::CloseCurrentPopup();
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
		{
			std::string sceneFile = GetSceneFilename(FileDialogs::OpenFile("scene"));
			if (sceneFile.size())
			{
				SceneManager::Load(sceneFile);
				SceneManager::SetActiveScene(sceneFile);
			}
		}

		ImGui::Dummy({ 0, 5.0f });
		ImGui::SetCursorPosX(ImGui::GetWindowWidth() / 2 - 105);

		// Save scene button
		if (ImGui::Button("Save", { 100, 25 }))
		{
			if (m_ActiveScene->m_SceneFilepath != "<Unsaved scene>")
			{
				SceneManager::SaveActiveScene();
				m_SavedSceneTextTimer = 2.0f;
			}
			else
				saveAs = true;
		}

		// Save as button
		ImGui::SameLine();
		if (ImGui::Button("Save as", { 100, 25 }))
			saveAs = true;

		// Save as scene button logic
		if (saveAs)
		{
			std::string sceneFile = GetSceneFilename(FileDialogs::SaveFile(".scene"));
			if (sceneFile.size())
			{
				SceneManager::SaveActiveSceneAs(sceneFile);
				if (m_ActiveScene->m_SceneFilepath == "<Unsaved scene>")
				{
					if (!createNewScene)
						SceneManager::Unload("<Unsaved scene>");
				}
				SceneManager::Load(sceneFile);
				SceneManager::SetActiveScene(sceneFile);
				m_SavedSceneTextTimer = 2.0f;
			}
		}

		// Create empty scene button logic
		if (createNewScene)
		{
			Scene* scene = SceneManager::CreateEmptyScene("<Unsaved scene>");
			SceneManager::s_Instance->m_ActiveScene = scene;
			EditorLayer::SetActiveScene(scene);
		}

		// Scenes memory view
		ImGui::Dummy({ 0, 5 });
		ImGui::Separator();
		ImGui::Dummy({ 0, 2 });
		if (ImGui::TreeNodeEx("Memory view", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::Dummy({ 0, 2 });
			for (auto& [sceneName, scenePtr] : SceneManager::s_Instance->m_Scenes)
			{
				ImGui::Text(sceneName.c_str());
				bool isActive = m_ActiveScene == scenePtr;

				ImGui::SameLine(ImGui::GetWindowWidth() - (isActive ? 140 : 160));
				if (!isActive && ImGui::Button(("Set active##" + sceneName).c_str()))
					SceneManager::SetActiveScene(sceneName);

				if (isActive)
				{
					ImGui::PushStyleColor(ImGuiCol_Text, { 0.0f, 1.0f, 0.2f, 1.0f });
					ImGui::Text("Currenty active");
					ImGui::PopStyleColor();
					ImGui::Dummy({ 0, 2 });
				}
				else
				{
					ImGui::SameLine();
					if (ImGui::Button(("Unload##" + sceneName).c_str()))
					{
						SceneManager::Unload(sceneName);
						break;
					}
				}
			}
			ImGui::TreePop();
		}
		ImGui::End();
	}


	void ScenePanel::OnUpdate(float ts) 
	{
		if (m_SavedSceneTextTimer == 2.0f)
			m_SavedSceneTextTimer = 1.999f;
		else
			m_SavedSceneTextTimer = glm::max(m_SavedSceneTextTimer - ts, 0.0f);
	}
}
