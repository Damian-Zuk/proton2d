#include "pch.h"
#include "proton/Editor/EditorOverlay.h"

#include <imgui.h>

namespace proton {

	EditorOverlay::EditorOverlay()
		: Layer("EditorOverlay")
	{
	}

	void EditorOverlay::OnAttach()
	{
	}

	void EditorOverlay::OnDetach()
	{
	}

	void EditorOverlay::OnUpdate(float ts)
	{
	}

	void EditorOverlay::OnImGuiRender()
	{
		ImGui::Begin("Scene");

		if (m_ActiveScene)
		{
			m_ActiveScene->m_Registry.each([&](auto id)
			{
				Entity entity{ m_ActiveScene.get(), id};

				ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_OpenOnArrow;
				if (m_Inspector.m_SelectedEntity == entity)
					flags |= ImGuiTreeNodeFlags_Selected;

				bool expanded = ImGui::TreeNodeEx((void*)(uint64_t)(uint32_t)entity, 
					flags, entity.GetComponent<TagComponent>().Tag.c_str());
				
				if (ImGui::IsItemClicked())
					m_Inspector.m_SelectedEntity = entity;

				if (expanded)
				{
					ImGui::TreePop();
				}

			});
		}

		ImGui::End();

		m_Inspector.OnImGuiRender();
	}

	void EditorOverlay::SetActiveScene(const Shared<Scene>& scene)
	{
		m_ActiveScene = scene;
		m_Inspector.m_ActiveScene = scene;
	}

}