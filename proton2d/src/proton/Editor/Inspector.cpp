#include "pch.h"
#include "proton/Editor/Inspector.h"

#include <imgui.h>

namespace proton {

	void Inspector::OnImGuiRender()
	{
		if (m_ActiveScene)
		{
			ImGui::Begin("Inspector");
			if (m_SelectedEntity)
			{
				if (m_SelectedEntity.HasComponent<TagComponent>())
				{
					DrawComponentUI<TagComponent>("Tag", [](auto& component)
					{
						char temp[256];
						strcpy_s(temp, sizeof(temp), component.Tag.c_str());
						if (ImGui::InputText("##Tag", temp, sizeof(temp)))
							component.Tag = std::string(temp);
					});
				}

				if (m_SelectedEntity.HasComponent<TransformComponent>())
				{
					DrawComponentUI<TransformComponent>("Transform", [](auto& component)
					{
						ImGui::Columns(2);
						ImGui::SetColumnWidth(0, 75.0f);
						ImGui::Text("Position");
						ImGui::NextColumn();
						ImGui::PushItemWidth(75.0f);
						ImGui::DragFloat("##P_X", &component.Position.x, 0.01f, 0.0f, 0.0f, "%.2f");
						ImGui::SameLine();
						ImGui::PushItemWidth(75.0f);
						ImGui::DragFloat("##P_Y", &component.Position.y, 0.01f, 0.0f, 0.0f, " % .2f");
						ImGui::SameLine();
						ImGui::PushItemWidth(75.0f);
						ImGui::DragFloat("##P_Z", &component.Position.z, 0.01f, 0.0f, 0.0f, "%.2f");
						ImGui::Columns(1);

						ImGui::Columns(2);
						ImGui::SetColumnWidth(0, 75.0f);
						ImGui::Text("Scale");
						ImGui::NextColumn();
						ImGui::PushItemWidth(75.0f);
						ImGui::DragFloat("##S_X", &component.Scale.x, 0.01f, 0.0f, 0.0f, "%.2f");
						ImGui::SameLine();
						ImGui::PushItemWidth(75.0f);
						ImGui::DragFloat("##S_Y", &component.Scale.y, 0.01f, 0.0f, 0.0f, "%.2f");
						ImGui::Columns(1);

						ImGui::Columns(2);
						ImGui::SetColumnWidth(0, 75.0f);
						ImGui::Text("Rotation");;
						ImGui::NextColumn();
						ImGui::PushItemWidth(75.0f);
						ImGui::DragFloat("##R", &component.Rotation, 0.1f, 0.0f, 0.0f, "%.2f");

						ImGui::Columns(1);
					});
				}
			}
			ImGui::End();
		}
	}

	template<typename T>
	void Inspector::DrawComponentUI(const std::string& name, void(*drawFunc)(T& component))
	{
		T& component = m_SelectedEntity.GetComponent<T>();

		ImGuiTreeNodeFlags treeNodeFlags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanAvailWidth
			| ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_FramePadding;
		

		bool expanded = ImGui::TreeNodeEx((void*)typeid(T).hash_code(), treeNodeFlags, name.c_str());

		if (expanded)
		{
			ImGui::Dummy(ImVec2(0.0f, 3.0f));
			drawFunc(component);
			ImGui::TreePop();
		}

		ImGui::Dummy(ImVec2(0.0f, 3.0f));
	}

}