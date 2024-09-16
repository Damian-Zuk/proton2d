#include "ptpch.h"
#ifdef PT_EDITOR
#include "Proton/Editor/Panels/ToolbarPanel.h"
#include "Proton/Editor/Panels/SceneViewportPanel.h"
#include "Proton/Editor/EditorLayer.h"

#include "Proton/Core/Application.h"
#include "Proton/Core/GameInstance.h"
#include "Proton/Scene/SceneManager.h"
#include "Proton/Utils/NickGenerator.h"

#include "Proton/Network/NetworkManager.h"

#include <imgui.h>
#include <imgui_internal.h>

static constexpr const char FontAwesome_Play[]   = u8"\uf04b";
static constexpr const char FontAwesome_Pause[]  = u8"\uf04c";
static constexpr const char FontAwesome_Resume[] = u8"\uf051";
static constexpr const char FontAwesome_Stop[]   = u8"\uf04d";
static constexpr const char FontAwesome_Rocket[] = u8"\uf135"; 
static constexpr const char FontAwesome_Dice[]   = u8"\uf522";

namespace proton {

	void ToolbarPanel::OnImGuiRender()
	{
		ImGuiWindowClass windowClass;
		windowClass.DockNodeFlagsOverrideSet = ImGuiDockNodeFlags_NoResize | ImGuiDockNodeFlags_NoTabBar;
		ImGui::SetNextWindowClass(&windowClass);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 0.0f, 0.0f });
		ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));
		ImGui::Begin("Toolbar", NULL, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollWithMouse);
		ImGui::PopStyleColor();
		ImGui::PopStyleVar();

		Scene* scene = GetActiveScene();

		if (!scene)
		{
			ImGui::End();
			return;
		}
		ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 8.0f);
		ImGui::Dummy({ 0, 5 });

		DrawSceneTabBar();

		float toolbarStartX = 50.0f;
		float toolbarStartY = ImGui::GetCursorPosY();

		ImGui::SetCursorPos({ 0.0f, toolbarStartY - 5.0f });
		ImDrawList* draw_list = ImGui::GetWindowDrawList();
		ImVec2 cursor = ImGui::GetCursorScreenPos();
		ImVec2 containerSize = ImVec2(ImGui::GetContentRegionAvail().x + 10.0f, 100.0f);
		ImU32 color = IM_COL32(45, 45, 45, 255);
		draw_list->AddRectFilled(cursor, ImVec2(cursor.x + containerSize.x, cursor.y + containerSize.y), color);

		ImGui::SetCursorPos({ toolbarStartX, toolbarStartY });

		ImGui::PushFont(EditorLayer::GetFontAwesome());
		ImGui::SetCursorPosX((ImGui::GetWindowWidth() - (scene->IsSimulated() ? 145 : 90)) / 2.0f);

		if (scene->IsSimulated())
		{
			// Stop button
			if (ImGui::Button(FontAwesome_Stop, { 60, 32 }))
			{
				EditorLayer::Get()->OnStopSimulationButton();
			}
			ImGui::SameLine();

			// Pause button
			if (ImGui::Button(scene->IsPaused() ? FontAwesome_Resume : FontAwesome_Pause, { 60, 32 }))
			{
				EditorLayer::Get()->OnPauseSimulationButton();
			}
		}
		// Play button
		else if (ImGui::Button(FontAwesome_Play, { 60, 32 }))
		{
			EditorLayer::Get()->OnStartSimulationButton();
		}

		ImGui::SameLine();
		if (ImGui::Button(FontAwesome_Rocket, {36, 32}))
		{
			m_LaunchInstanceProps.OpenPopup = true;
			UpdateInstancePropsWindowTitle();
		}
		ImGui::PopFont();

		auto viewport = EditorLayer::GetFocusedViewportPanel();
		if (!viewport->m_IsMainViewport)
		{
			ImVec2 textSize = ImGui::CalcTextSize(viewport->m_ImGuiWindowName.c_str());
			ImGui::SameLine();
			ImGui::SetCursorPosX(ImGui::GetContentRegionMax().x - textSize.x - 10.0f);
			ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 2.0f);
			ImGui::TextColored({ 1.0f, 0.8f, 0.12f, 1.0f }, "%s", viewport->m_ImGuiWindowName.c_str());
		}
		
		HandleLaunchInstancePopup();

		ImGui::End();
	}

	void ToolbarPanel::DrawSceneTabBar()
	{	
		ImGui::SetCursorPosX(30.0f);
		if (ImGui::BeginTabBar("SceneTabBar", ImGuiTabBarFlags_AutoSelectNewTabs)) 
		{
			SceneManager* manager = EditorLayer::GetMainGameInstance()->GetSceneManager();
			const std::string& activeSceneName = manager->GetActiveScene()->GetFilepath();

			for (auto& [name, scene] : manager->m_Scenes)
			{
				bool keepOpen = true;
				bool selected = activeSceneName == name;
				ImGuiTabBarFlags flags = selected ? ImGuiTabItemFlags_SetSelected : ImGuiTabItemFlags_None;

				bool result = ImGui::BeginTabItem(name.c_str(), &keepOpen, flags);

				if (result)
					ImGui::EndTabItem();

				if (ImGui::IsItemClicked() && !selected)
					manager->SetActiveScene(name);

				if (!keepOpen)
				{
					manager->Unload(name);
					break;
				}
			}
			ImGui::EndTabBar();
		}
	}

	void ToolbarPanel::HandleLaunchInstancePopup()
	{
		char buffer[256];
		auto& props = m_LaunchInstanceProps;

		if (props.OpenPopup)
			ImGui::OpenPopup("Launch Game Instance");

		if (ImGui::BeginPopupModal("Launch Game Instance", NULL, ImGuiWindowFlags_AlwaysAutoResize))
		{
			constexpr char* netModesNames[] = { "Standalone", "Listen Server", "Dedicated Server", "Client" };
			NetMode& netMode = props.NetMode;

			ImGui::Dummy({ 0, 5 });
			ImGui::PushItemWidth(200.0f);
			if (ImGui::BeginCombo("Net Mode", netModesNames[(uint8_t)netMode]))
			{
				for (uint8_t i = 0; i < 4; i++)
				{
					bool selected = (uint8_t)netMode == i;
					if (ImGui::Selectable(netModesNames[i], selected))
					{
						netMode = (NetMode)i;
						UpdateInstancePropsWindowTitle();
					}
				}
				ImGui::EndCombo();
			}
			ImGui::Dummy({ 0, 5 });
			ImGui::PopItemWidth();

			if (netMode == NetMode::Client || netMode == NetMode::ListenServer)
			{
				ImGui::PushItemWidth(180.0f);

				std::strncpy(buffer, props.ClientNickname.c_str(), sizeof(buffer));
				if (ImGui::InputText("Nick", buffer, sizeof(buffer)))
					props.ClientNickname = buffer;

				ImGui::SameLine();
				ImGui::PushFont(EditorLayer::GetFontAwesome());
				if (ImGui::Button(FontAwesome_Dice, ImVec2(40, 25)))
					props.ClientNickname = GenerateRandomNickname();
				ImGui::PopFont();

				if (netMode == NetMode::Client)
				{
					std::strncpy(buffer, props.ServerIp.c_str(), sizeof(buffer));
					if (ImGui::InputText("Server IP", buffer, sizeof(buffer)))
						props.ServerIp = buffer;

					ImGui::DragInt("Port", &props.Port, 1.0f, 0, 65535);
				}
				ImGui::PopItemWidth();
			}

			if (netMode != NetMode::Standalone)
			{
				ImGui::PushItemWidth(180.0f);
				if (ImGui::DragInt("Tick Rate (hz)", &props.ServerTickrate, 1.0f, 1, 256))
					props.ServerTickrate = std::clamp(props.ServerTickrate, 1, 256);
				bool temp = false;
				ImGui::Checkbox("Use Physics Tickrate", &temp);
				ImGui::PopItemWidth();
			}

			if (netMode == NetMode::ListenServer || netMode == NetMode::DedicatedServer)
			{	
				ImGui::PushItemWidth(180.0f);
				ImGui::DragInt("Port", &props.Port, 1.0f, 0, 65535);
				ImGui::PopItemWidth();
			}
			
			if (netMode != NetMode::Standalone)
				ImGui::Dummy({ 0, 5 });

			if (ImGui::TreeNode("More options"))
			{
				ImGui::Dummy({ 0, 2 });
				ImGui::PushItemWidth(160.0f);
				std::strncpy(buffer, props.WindowName.c_str(), sizeof(buffer));
				if (ImGui::InputText("Window Name", buffer, sizeof(buffer)))
					props.WindowName = buffer;
				ImGui::PopItemWidth();
				ImGui::Checkbox("Load Startup Scene", &props.LoadStartupScene);
				ImGui::TreePop();
			}

			ImGui::Dummy({ 0, 5 });
			if (ImGui::Button("Launch", { 150, 0 }))
			{
				auto instance = EditorLayer::Get()->LaunchNewGameInstance(netMode, props.LoadStartupScene, props.WindowName);
				auto netManager = instance->GetNetworkManager();

				if (netMode == NetMode::Client)
				{
					netManager->SetIpAddress(props.ServerIp);
					netManager->SetNetworkPort(props.Port);
				}
				else if (netMode == NetMode::ListenServer || netMode == NetMode::DedicatedServer)
				{
					netManager->SetTickRate(props.ServerTickrate);
					netManager->SetNetworkPort(props.Port);
				}


				instance->GetActiveScene()->BeginPlay();
				ImGui::CloseCurrentPopup();
			}
			ImGui::SetItemDefaultFocus();
			ImGui::SameLine();
			if (ImGui::Button("Cancel", { 150, 0 })) { ImGui::CloseCurrentPopup(); }

			ImGui::EndPopup();
			props.OpenPopup = false;
		}

		if (!props.ClientNickname.size())
			props.ClientNickname = GenerateRandomNickname();
	}

	void ToolbarPanel::UpdateInstancePropsWindowTitle()
	{
		auto& props = m_LaunchInstanceProps;
		props.WindowName = NetModeToString(props.NetMode) + " " + std::to_string(EditorLayer::Get()->m_CurrentInstanceID + 1);
	}

}
#endif // PT_EDITOR
