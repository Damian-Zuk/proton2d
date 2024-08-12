#include "ptpch.h"
#ifdef PT_EDITOR
#include "Proton/Editor/Panels/InfoPanel.h"
#include "Proton/Editor/EditorLayer.h"

#include "Proton/Core/Application.h"
#include "Proton/Core/GameInstance.h"
#include "Proton/Core/AssetManager.h"
#include "Proton/Graphics/Renderer/Renderer.h"

#include "Proton/Network/NetworkManager.h"
#include "Proton/Network/Server.h"
#include "Proton/Network/NetStatistics.h"

#include "imgui.h"

namespace proton {

	static constexpr float s_StatsRefreshInterval = 0.2f;

	void InfoPanel::OnImGuiRender()
	{
		m_FrameTime = Application::GetLastFrameTime() * 1000.0f;

		if (m_RefreshStatsTimer <= 0.0f)
		{
			m_FrameTimeDisplay = m_FrameTime;

			m_FrameTimeHistory[m_FrameTimeValuesOffset] = m_FrameTime;
			m_FrameTimeValuesOffset = (m_FrameTimeValuesOffset + 1) % s_FrameTimePlotValuesCount;

			m_FPS = m_FrameCount ? (m_FrameCount - 1) / (s_StatsRefreshInterval - m_RefreshStatsTimer) : 0.0f;
			m_FrameCount = 0;

			float sum = 0.0f; int total = 0;
			for (uint32_t i = 0; i < s_FrameTimePlotValuesCount; i++)
			{
				if (m_FrameTimeHistory[i] != 0.0f)
				{
					sum += m_FrameTimeHistory[i];
					total++;
				}
			}

			if (total)
				m_AvgFrameTime = sum / (float)total;

			m_RefreshStatsTimer = s_StatsRefreshInterval;
		}
		else
			m_RefreshStatsTimer -= m_FrameTime / 1000.0f;

		ImGui::Begin("Info");

		Scene* scene = GetActiveScene(true);

		if (ImGui::TreeNodeEx("General", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::Dummy({ 0, 5 });
			uint32_t entitiesCount = scene ? scene->GetEntitiesCount() : 0;
			uint32_t scriptedEntitiesCount = scene ? scene->GetScriptedEntitiesCount() : 0;
			uint32_t networkedEntitiesCount = scene ? scene->GetNetworkedEntitiesCount() : 0;
			ImGui::Text("Entities: %i (%i scripted, %i networked)", entitiesCount, scriptedEntitiesCount, networkedEntitiesCount);
			//ImGui::Text("OpenGL Draw Calls: %i", scene ? Renderer::GetDrawCallsCount() : 0);

			ImGui::Dummy({ 0, 5 });
			ImGui::Text("Frame Time: %0.3f ms. (%.2f FPS)", m_FrameTimeDisplay, m_FPS);

			float max = 0.0f;
			for (uint32_t i = 0; i < s_FrameTimePlotValuesCount; i++)
				max = m_FrameTimeHistory[i] > max ? m_FrameTimeHistory[i] : max;

			ImGui::Text("   max:\n%0.3f\n   avg:\n%0.3f", max, m_AvgFrameTime);
			ImGui::SameLine();
			ImGui::PlotLines("##Frame_Time", m_FrameTimeHistory, s_FrameTimePlotValuesCount, m_FrameTimeValuesOffset, NULL, 0.0f, glm::max(max * 1.1f, 1.0f / 60.0f), ImVec2(0, 80));
			ImGui::TreePop();
		}

		ImGui::Dummy({ 0, 15 });
		if (ImGui::TreeNodeEx("Network Statistics", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::Dummy({ 0, 5 });
			DrawNetworkStats();
			ImGui::TreePop();
		}
		
		ImGui::Dummy({ 0, 15 });
		if (ImGui::TreeNodeEx("Debug", ImGuiTreeNodeFlags_DefaultOpen))
		{
			if (scene)
			{
				auto& position = scene->GetPrimaryCameraPosition();
				ImGui::Text("Camera Position: (%.2f, %.2f)", position.x, position.y);
			}
			ImGui::TreePop();
		}

		ImGui::End();
		m_FrameCount++;
	}

	void InfoPanel::DrawNetworkStats()
	{
		static HSteamNetConnection selectedConn = 0;
		NetworkManager* networkManager = EditorLayer::GetMainGameInstance()->GetNetworkManager();
		Server* server = networkManager->GetServer();

		ImGui::Checkbox("Log To File", &networkManager->m_SaveNetworkStatsToLogFile);
		if (networkManager->m_SaveNetworkStatsToLogFile)
		{
			ImGui::SameLine(); ImGui::Dummy({ 3, 0 }); ImGui::SameLine();
			ImGui::Checkbox("All Clients", &networkManager->m_SaveStatsForAllClients);
		}
		ImGui::Dummy({ 0, 5 });

		if (!networkManager->IsNetModeServer() || !networkManager->IsNetworkActive())
		{
			ImGui::Text("Stats not available: Server is not running.");
			return;
		}

		const auto& statsAll = server->m_NetStatistics->GetNetworkStats();

		if (statsAll.empty())
		{
			ImGui::Text("Stats not available: No clients connected.");
			selectedConn = 0;
			return;
		}

		if (selectedConn == 0 || statsAll.find(selectedConn) == statsAll.end())
			selectedConn = statsAll.begin()->first;

		networkManager->m_SaveStatsForClientID = selectedConn;

		ImGui::PushItemWidth(150.0f);
		if (ImGui::BeginCombo("Client Connection", std::to_string(selectedConn).c_str()))
		{
			for (const auto& [hConn, stats] : statsAll)
			{
				if (ImGui::Selectable(std::to_string(hConn).c_str(), selectedConn == hConn))
				{
					selectedConn = hConn;
				}
			}
			ImGui::EndCombo();
		}
		ImGui::PopItemWidth();

		auto& stats = statsAll.at(selectedConn);

		if (selectedConn != 0)
		{
			ImGui::Dummy({ 0, 5 });
			if (ImGui::TreeNodeEx("Summary", ImGuiTreeNodeFlags_DefaultOpen))
			{
				ImGui::Dummy({ 0, 3 });

				ImGui::Columns(3);
				ImGui::SetColumnWidth(0, 75.0f);
				ImGui::SetColumnWidth(1, 100.0f);
				ImGui::SetColumnWidth(2, 75.0f);

				ImGui::Text("Ping"); ImGui::NextColumn();
				ImGui::Text("%i", stats.RealTime.m_nPing); ImGui::NextColumn(); 
				ImGui::Text("ms"); ImGui::NextColumn();

				ImGui::Text("In"); ImGui::NextColumn();
				ImGui::Text("%.1f", stats.RealTime.m_flInBytesPerSec); ImGui::NextColumn();
				ImGui::Text("Bytes/s"); ImGui::NextColumn();

				ImGui::Text("Out"); ImGui::NextColumn();
				ImGui::Text("%.1f", stats.RealTime.m_flOutBytesPerSec); ImGui::NextColumn();
				ImGui::Text("Bytes/s"); ImGui::NextColumn();
				
				ImGui::Columns(1);

				ImGui::TreePop();
			}

			ImGui::Dummy({ 0, 15 });
			if (ImGui::TreeNodeEx("Details", ImGuiTreeNodeFlags_DefaultOpen))
			{
				ImGui::Dummy({ 0, 3 });
				ImGui::Text((char*)stats.Detailed.Data);
				ImGui::TreePop();
			}
		}

	}

}

#endif // PT_EDITOR
