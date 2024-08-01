#include "ptpch.h"
#ifdef PT_EDITOR
#include "Proton/Editor/EditorLayer.h"
#include "Proton/Editor/Panels/SettingsPanel.h"
#include "Proton/Editor/Panels/SceneViewportPanel.h"
#include "Proton/Editor/Panels/InspectorPanel.h"
#include "Proton/Graphics/Renderer/Renderer.h"
#include "Proton/Assets/AssetManager.h"
#include "Proton/Core/Application.h"
#include "Proton/Core/ProjectSettings.h"
#include "Proton/Core/GameInstance.h"
#include "Proton/Network/Common/NetworkManager.h"
#include "Proton/Network/Server/Server.h"

#include "imgui.h"

namespace proton {

	static constexpr float s_StatsRefreshInterval = 0.2f;

	void SettingsPanel::OnImGuiRender()
	{
		ImGui::Begin("Settings");
		
		// Get selected game instance
		SceneViewportPanel* viewportPanel = EditorLayer::GetFocusedViewportPanel();
		Scene* activeScene = GetActiveScene(false);

		if (activeScene && ImGui::TreeNodeEx("Network", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::Dummy({ 0, 2 });
			
			NetworkManager* networkManager = activeScene->m_GameInstance->GetNetworkManager();

			constexpr char* netModesNames[] = { "Standalone", "Listen Server" };
			const NetMode netMode = Application::GetGameInstance()->GetNetMode();

			ImGui::PushItemWidth(160.0f);
			if (ImGui::BeginCombo("Net Mode", netModesNames[(uint8_t)netMode]))
			{
				for (uint8_t i = 0; i < 2; i++)
				{
					bool selected = (uint8_t)netMode == i;
					if (ImGui::Selectable(netModesNames[i], selected))
					{
						networkManager->SetNetMode((NetMode)i);
					}
				}
				ImGui::EndCombo();
			}
			ImGui::PopItemWidth();

			if (netMode == NetMode::ListenServer)
			{
				ImGui::PushItemWidth(95.0f);

				int tickRate = networkManager->m_ServerTickRate;
				if (ImGui::DragInt("Tick Rate (hz)", &tickRate, 1, 1, 256))
				{
					networkManager->SetServerTickRate(glm::max(tickRate, 1));
				}
				ImGui::PopItemWidth();
			}

			ImGui::PushItemWidth(95.0f);
			if (netMode == NetMode::ListenServer &&
				ImGui::DragFloat("Fake Lag (ms)", &Server::s_FakeServerLag, 0.1f, 0.0f, 500.0f, "%.0f"))
			{
				if (Server* server = networkManager->GetServer())
					server->SetPacketFakeLag(Server::s_FakeServerLag);
			}
			ImGui::PopItemWidth();

			ImGui::Checkbox("Trace Entity Net Sync", &viewportPanel->m_ShowNetPosition);

			ImGui::TreePop();
		}
		ImGui::Dummy({ 0, 5 });

		if (ImGui::TreeNodeEx("Game Viewport", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::Dummy({ 0, 2 });
			//ImGui::Checkbox("Selection Outline", &viewportPanel->m_ShowSelectionOutline);
			ImGui::Checkbox("Show Entity Collider", &viewportPanel->m_ShowSelectionCollider);
			ImGui::Checkbox("Show All Colliders", &viewportPanel->m_ShowAllColliders);
			ImGui::Checkbox("Simulation Freecam", &viewportPanel->m_Camera->m_UseInRuntime);
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
