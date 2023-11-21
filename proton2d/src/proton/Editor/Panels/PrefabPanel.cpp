#include "pch.h"
#include "proton/Editor/Panels/PrefabPanel.h"
#include "proton/Scene/PrefabManager.h"

#include <imgui.h>

namespace proton {
	
	void PrefabPanel::OnImGuiRender()
	{
		ImGui::Begin("Prefabs");
		ImGui::Dummy({ 0, 1 });
		if (ImGui::Button("Refresh list"))
			PrefabManager::ReloadAllPrefabs();

		std::string deletePrefabTag;
		ImGui::Dummy({ 0.0f, 5.0f });
		for (auto& [tag, jsonData] : PrefabManager::s_Instance->m_PrefabsJsonData)
		{
			ImGui::Separator();
			ImGui::Text(tag.c_str());
			ImGui::SameLine(ImGui::GetWindowWidth() - 120);
			if (ImGui::Button(("Spawn##" + tag).c_str(), { 60, 25 }))
			{
				Entity entity = PrefabManager::SpawnPrefab(m_ActiveScene, tag);
				auto& transform = entity.GetComponent<TransformComponent>();
				glm::vec2 cameraPos = m_ActiveScene->GetPrimaryCameraPosition();
				transform.Position.x = cameraPos.x;
				transform.Position.y = cameraPos.y;
				if (m_SelectedEntity)
					m_SelectedEntity.AddChildEntity(entity);
			}
			ImGui::SameLine();
			if (ImGui::Button(("X##" + tag).c_str(), { 25, 25 }))
				deletePrefabTag = tag;
		}
		if (deletePrefabTag.size())
			PrefabManager::DeletePrefab(deletePrefabTag);
		ImGui::Separator();
		ImGui::End();
	}
}
