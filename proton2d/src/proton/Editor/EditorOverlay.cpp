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
		ImGui::Begin("Scene Entity Nodes");

		if (m_ActiveScene)
		{
			m_ActiveScene->m_Registry.each([&](auto id)
			{
				Entity entity{ m_ActiveScene.get(), id};

				ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_OpenOnArrow;
				if (m_SelectedEntity == entity)
					flags |= ImGuiTreeNodeFlags_Selected;

				auto& tag = entity.GetComponent<TagComponent>().Tag;
				bool expanded = ImGui::TreeNodeEx((void*)(uint64_t)(uint32_t)entity, flags, tag.c_str());
				
				if (ImGui::IsItemClicked())
				{
					m_SelectedEntity = entity;
				}

				if (expanded)
				{
					ImGui::TreePop();
				}

			});
		}

		ImGui::End();
	}

	void EditorOverlay::SetActiveScene(const Shared<Scene>& scene)
	{
		m_ActiveScene = scene;
	}

}