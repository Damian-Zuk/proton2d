#include "ptpch.h"
#ifdef PT_EDITOR
#include "Proton/Editor/Panels/ToolbarPanel.h"
#include "Proton/Editor/EditorLayer.h"
#include "Proton/Core/Application.h"
#include "Proton/Core/GameInstance.h"
#include "Proton/Scene/SceneManager.h"
#include "Proton/Assets/SceneSerializer.h"
#include "Proton/Physics/PhysicsWorld.h"
#include "Proton/Utils/Utils.h"

#include <imgui.h>

static constexpr const char FontAwesome_Play[]   = u8"\uf04b";
static constexpr const char FontAwesome_Pause[]  = u8"\uf04c";
static constexpr const char FontAwesome_Resume[] = u8"\uf051";
static constexpr const char FontAwesome_Stop[]   = u8"\uf04d";

namespace proton {

	void ToolbarPanel::OnImGuiRender()
	{
		ImGui::Begin("Toolbar", NULL, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollWithMouse);
		
		Scene* activeScene = GetActiveScene();

		if (!activeScene)
		{
			ImGui::End();
			return;
		}

		ImGui::PushFont(EditorLayer::GetFontAwesome());
		ImGui::SetCursorPosX((ImGui::GetWindowWidth() - (!activeScene->IsSimulated() ? 60 : 145)) / 2.0f);

		if (activeScene->IsSimulated())
		{
			if (ImGui::Button(FontAwesome_Stop, { 60, 32 }))
			{
				EditorLayer::Get()->OnStopButton();
			}
			ImGui::SameLine();

			if (ImGui::Button(activeScene->IsPaused() ? FontAwesome_Resume : FontAwesome_Pause, {60, 32}))
			{
				EditorLayer::Get()->OnPauseButton();
			}
		}
		else if (ImGui::Button(FontAwesome_Play, { 60, 32 }))
		{
			EditorLayer::Get()->OnPlayButton();
		}
		ImGui::PopFont();
		DrawSceneTabBar();

		ImGui::End();
	}

	void ToolbarPanel::DrawSceneTabBar()
	{
		if (ImGui::BeginTabBar("SceneTabBar", ImGuiTabBarFlags_AutoSelectNewTabs)) 
		{
			SceneManager* manager = Application::GetGameInstance()->GetSceneManager();
			const std::string& activeScene = manager->GetActiveScene()->GetFilepath();

			for (auto& [name, scene] : manager->m_Scenes)
			{
				bool keepOpen = true;
				bool selected = activeScene == name;
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

}
#endif
