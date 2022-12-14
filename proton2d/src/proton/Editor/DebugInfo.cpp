#include "pch.h"
#include "proton/Editor/DebugInfo.h"
#include "proton/Graphics/Renderer.h"
#include "proton/Editor/EditorOverlay.h"

#include "imgui.h"

namespace proton {

	static constexpr float s_StatsRefreshInterval = 0.2f;

	void DebugInfo::OnImGuiRender()
	{
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
			m_RefreshStatsTimer -= m_FrameTime;

		ImGui::Begin("Settings and statistics");

		if (ImGui::TreeNodeEx("Settings", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::Checkbox("Show selection outline", &EditorOverlay::s_Instance->m_ShowSelectionOutline);
			ImGui::Checkbox("Show selection collider", &EditorOverlay::s_Instance->m_ShowSelectionCollider);
			ImGui::Checkbox("Show all colliders", &EditorOverlay::s_Instance->m_ShowAllColliders);
			ImGui::TreePop();
		}

		ImGui::Dummy({ 0.0f, 3.0f });
		ImGui::Text("Frame time: %f sec. (%.2f FPS)", m_FrameTimeDisplay, m_FPS);

		float max = 0.0f;
		for (uint32_t i = 0; i < s_FrameTimePlotValuesCount; i++)
			max = m_FrameTimeHistory[i] > max ? m_FrameTimeHistory[i] : max;

		ImGui::Text(" max (s):\n%f\n avg (s):\n%f", max, m_AvgFrameTime);
		ImGui::SameLine();
		ImGui::PlotLines("##Frame_Time", m_FrameTimeHistory, s_FrameTimePlotValuesCount, m_FrameTimeValuesOffset, NULL, 0.0f, glm::max(max * 1.1f, 1.0f / 60.0f), ImVec2(0, 80));
		ImGui::Dummy({ 0.0f, 2.0f });
		ImGui::Text("Entities: %i (scripted: %i)", m_EntitiesCount, m_ScriptedEntitiesCount);
		ImGui::Text("OpenGL draw calls: %i", Renderer::GetDrawCallsCount());
		
		ImGui::End();

		m_FrameCount++;
	}

}
