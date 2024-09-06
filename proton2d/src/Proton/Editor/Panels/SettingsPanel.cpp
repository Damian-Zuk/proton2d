#include "ptpch.h"
#ifdef PT_EDITOR
#include "Proton/Editor/EditorLayer.h"
#include "Proton/Editor/Panels/SettingsPanel.h"
#include "Proton/Editor/Panels/SceneViewportPanel.h"
#include "Proton/Editor/Panels/InspectorPanel.h"
#include "Proton/Editor/Tools/EditorCamera.h"

#include "Proton/Core/Application.h"
#include "Proton/Core/GameInstance.h"
#include "Proton/Core/ProjectSettings.h"
#include "Proton/Utils/NickGenerator.h"

#include "Proton/Network/NetworkManager.h"
#include "Proton/Network/Server.h"

#include "imgui.h"

namespace proton {

	static constexpr const char FontAwesome_Dice[] = u8"\uf522";

	void SettingsPanel::OnImGuiRender()
	{
		ImGui::Begin("Settings");
		
		// Get selected game instance
		SceneViewportPanel* viewportPanel = EditorLayer::GetFocusedViewportPanel();
		Scene* scene = GetActiveScene();

		if (scene && ImGui::TreeNodeEx("Network", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::Dummy({ 0, 2 });
			
			NetworkManager* networkManager = scene->m_GameInstance->GetNetworkManager();

			constexpr char* netModesNames[] = { "Standalone", "Listen Server", "Dedicated Server", "Client"};
			const NetMode netMode = scene->m_GameInstance->GetNetMode();

			ImGui::PushItemWidth(200.0f);
			if (ImGui::BeginCombo("Net Mode", netModesNames[(uint8_t)netMode]))
			{
				for (uint8_t i = 0; i < 4; i++)
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

			if (netMode != NetMode::Standalone)
			{
				ImGui::PushItemWidth(200.0f);
				if (netMode != NetMode::DedicatedServer)
				{
					static char clientNameBuffer[32];
					strcpy_s(clientNameBuffer, networkManager->m_ClientName.substr(0, 31).c_str());
					if (ImGui::InputText("Nick", clientNameBuffer, 32))
						networkManager->SetLocalClientName(clientNameBuffer);

					ImGui::SameLine();
					ImGui::PushFont(EditorLayer::GetFontAwesome());
					if (ImGui::Button(FontAwesome_Dice, ImVec2(40, 25)))
						networkManager->SetLocalClientName(GenerateRandomNickname());
					ImGui::PopFont();
				}
				ImGui::PopItemWidth();

				ImGui::PushItemWidth(200.0f);
				if (netMode == NetMode::Client)
				{
					static char ipBuffer[64];
					strcpy_s(ipBuffer, networkManager->m_IpAddress.c_str());
					if (ImGui::InputText("IP Address", ipBuffer, 64))
						networkManager->m_IpAddress = ipBuffer;
				}
				ImGui::PopItemWidth();

				ImGui::PushItemWidth(95.0f);
				int portValue = networkManager->m_Port;
				if (ImGui::DragInt("Network Port", &portValue, 1.0f, 0, 65535))
					networkManager->SetNetworkPort(portValue);
				ImGui::PopItemWidth();
			}

			ImGui::PushItemWidth(95.0f);
			if (netMode == NetMode::ListenServer || netMode == NetMode::DedicatedServer)
			{
				int maxConnections = networkManager->m_MaxServerConnections;
				if (ImGui::DragInt("Max Connections", &maxConnections))
					networkManager->m_MaxServerConnections = (uint32_t)glm::max(maxConnections, 0);

				if (ImGui::DragFloat("Fake Lag (ms)", &Server::s_FakeServerLag, 0.1f, 0.0f, 500.0f, "%.0f"))
				{
					if (Server* server = networkManager->GetServer())
						server->SetPacketFakeLag(Server::s_FakeServerLag);
				}
			}
			ImGui::PopItemWidth();

			if (netMode != NetMode::Standalone)
			{
				if (!networkManager->m_UsePhysicsTickrate)
				{
					int tickRate = networkManager->m_Tickrate;
					ImGui::PushItemWidth(95.0f);
					if (ImGui::DragInt("Tick Rate (hz)", &tickRate, 1, 1, 256))
						networkManager->SetTickRate(glm::max(tickRate, 1));
					ImGui::PopItemWidth();
				}

				ImGui::Checkbox("Use Physics Tickrate", &networkManager->m_UsePhysicsTickrate);
			}
			
			if (netMode != NetMode::Standalone)
			{
				ImGui::Dummy({ 0, 5 });
				ImGui::Separator();
				ImGui::Dummy({ 0, 5 });
			}

			if (netMode == NetMode::ListenServer || netMode == NetMode::DedicatedServer)
			{
				ImGui::Checkbox("Autostart Client", &EditorLayer::Get()->m_AutostartClient);
			}

			if (netMode == NetMode::Client)
				ImGui::Checkbox("Trace Entity Synchronization", &viewportPanel->m_TraceEntitySync);
			
			if (netMode != NetMode::Standalone)
			{
				ImGui::Checkbox("Trace Entity Cull Distance", &viewportPanel->m_ShowCullDistance);
			}

			ImGui::TreePop();
		}

		ImGui::Dummy({ 0, 10 });
		if (ImGui::TreeNodeEx("Game Viewport", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::Dummy({ 0, 2 });
			ImGui::Checkbox("Selection Outline", &viewportPanel->m_ShowSelectionOutline);
			ImGui::Checkbox("Show Entity Collider", &viewportPanel->m_ShowSelectionCollider);
			ImGui::Checkbox("Show All Colliders", &viewportPanel->m_ShowAllColliders);
			ImGui::Checkbox("Simulation Freecam", &viewportPanel->m_Camera->m_UseInRuntime);
			ImGui::TreePop();
		}

		ImGui::Dummy({ 0, 10 });
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

#endif // PT_EDITOR
