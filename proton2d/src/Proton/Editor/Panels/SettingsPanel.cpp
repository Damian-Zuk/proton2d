#include "ptpch.h"
#ifdef PT_EDITOR
#include "Proton/Editor/EditorLayer.h"
#include "Proton/Editor/Panels/SettingsPanel.h"
#include "Proton/Editor/Panels/SceneViewportPanel.h"
#include "Proton/Graphics/Renderer/Renderer.h"
#include "Proton/Assets/AssetManager.h"
#include "Proton/Core/Application.h"
#include "Proton/Core/ProjectSettings.h"
#include "Proton/Core/GameInstance.h"
#include "Proton/Network/Common/NetworkManager.h"

#include "imgui.h"

namespace proton {

	static constexpr float s_StatsRefreshInterval = 0.2f;

	void SettingsPanel::OnImGuiRender()
	{
		ImGui::Begin("Settings");

		ProjectSettings& project = Application::Get().GetGameInstance()->m_ProjectSettings;

		char buffer[256];
		strcpy_s(buffer, project.m_StartScene.c_str());

		if (ImGui::TreeNodeEx("Project", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::Dummy({ 0, 2 });
			ImGui::Text("Start Scene");
			ImGui::SameLine();
			ImGui::PushItemWidth(120.0f);
			if (ImGui::InputText("##start_scene", buffer, 256))
			{
				project.m_StartScene = buffer;
			}
		
			ImGui::SameLine();
			if (ImGui::Button("Set"))
			{
				project.WriteProjectSettings();
			}

			ImGui::TreePop();
		}
		ImGui::Dummy({ 0, 5 });

		if (ImGui::TreeNodeEx("Network", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::Dummy({ 0, 2 });
			constexpr char* netModesNames[] = { "Standalone", "Listen Server" };
			const NetMode netMode = Application::GetGameInstance()->GetNetMode();

			ImGui::Text("Net Mode");
			ImGui::SameLine();
			ImGui::PushItemWidth(150.0f);
			if (ImGui::BeginCombo("##net_mode", netModesNames[(uint8_t)netMode]))
			{
				for (uint8_t i = 0; i < 2; i++)
				{
					bool selected = (uint8_t)netMode == i;
					if (ImGui::Selectable(netModesNames[i], selected))
					{
						Application::GetGameInstance()->SetNetMode((NetMode)i);
					}
				}
				ImGui::EndCombo();
			}
			ImGui::PopItemWidth();

			if (netMode == NetMode::ListenServer)
			{
				int numClients = EditorLayer::Get()->m_NetNumClients;
				int count = numClients;
				if (ImGui::InputInt("Number of Clients", &count, 1, 1))
				{
					if (count > numClients)
						EditorLayer::Get()->OnAddClientButton();
					else
						EditorLayer::Get()->OnRemoveClientButton();
				}

				NetworkManager* networkManager = m_ActiveScene->m_GameInstance->GetNetworkManager();
				int tickRate = networkManager->m_ServerTickRate;
				if (ImGui::SliderInt("Server Tick Rate", &tickRate, 1, 128))
				{
					networkManager->SetServerTickRate(tickRate);
				}
			}

			ImGui::TreePop();
		}

		ImGui::Dummy({ 0, 5 });

		if (ImGui::TreeNodeEx("Editor", ImGuiTreeNodeFlags_DefaultOpen))
		{
			SceneViewportPanel* viewportPanel = EditorLayer::GetSceneViewportPanel();
			ImGui::Dummy({ 0, 2 });
			ImGui::Checkbox("Selection Outline", &viewportPanel->m_ShowSelectionOutline);
			ImGui::Checkbox("Selection Collider", &viewportPanel->m_ShowSelectionCollider);
			ImGui::Checkbox("Show Colliders", &viewportPanel->m_ShowAllColliders);
			ImGui::Checkbox("Runtime Camera", &EditorLayer::GetCamera()->m_UseInRuntime);
			ImGui::TreePop();
		}
		ImGui::Dummy({ 0, 5 });

		if (ImGui::TreeNodeEx("Application", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::Dummy({ 0, 2 });
			Window& window = Application::Get().GetWindow();
			bool fullscreen = window.IsFullscreen();
			if (ImGui::Checkbox("Fullscreen", &fullscreen))
				window.SetFullscreen(fullscreen);
			bool vsync = Application::Get().GetWindow().IsVSync();
			if (ImGui::Checkbox("VSync", &vsync))
				window.SetVSync(vsync);

			ImGui::PushItemWidth(100.0f);
			float timeScale = Application::Get().m_TimeScale;
			if (ImGui::DragFloat("Time Scale", &timeScale, 0.01f, 0.0f)
				&& timeScale >= 0.0f)
			{
				Application::Get().m_TimeScale = timeScale;
			}
			ImGui::PopItemWidth();
			ImGui::TreePop();
		}

		ImGui::End();
	}

}

#endif PT_EDITOR
