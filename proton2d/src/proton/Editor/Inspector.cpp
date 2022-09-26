#include "pch.h"
#include "proton/Editor/Inspector.h"
#include "proton/Assets/AssetsManager.h"

#include <imgui.h>
#include <glm/gtc/type_ptr.hpp>

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
						char buffer[256];
						strcpy_s(buffer, sizeof(buffer), component.Tag.c_str());
						if (ImGui::InputText("##Tag", buffer, sizeof(buffer)))
							component.Tag = std::string(buffer);
					});
				}

				if (m_SelectedEntity.HasComponent<TransformComponent>())
				{
					DrawComponentUI<TransformComponent>("Transform", [](auto& component)
					{
						// Draw position control
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

						// Draw Scale control
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

						// Draw Rotation control
						ImGui::Columns(2);
						ImGui::SetColumnWidth(0, 75.0f);
						ImGui::Text("Rotation");;
						ImGui::NextColumn();
						ImGui::PushItemWidth(75.0f);
						ImGui::DragFloat("##R", &component.Rotation, 0.1f, 0.0f, 0.0f, "%.2f");

						ImGui::Columns(1);
					});
				}

				if (m_SelectedEntity.HasComponent<SpriteComponent>())
				{
					DrawComponentUI<SpriteComponent>("Sprite", [&](auto& component)
					{
						auto& sprite = component.Sprite;
						
						// If component has sprite assigned then store it's path
						if (sprite && !m_SpriteComponentTextureSource.size())
							m_SpriteComponentTextureSource = sprite->GetTexture()->GetPath();

						// Texutre source input
						char buffer[256];
						strcpy_s(buffer, sizeof(buffer), m_SpriteComponentTextureSource.c_str());
						ImGui::Text("Texture source:");
						ImGui::Dummy(ImVec2(0.0f, 3.0f));
						ImGui::PushItemWidth(280.0f);
						ImGui::InputText("##Texture_Source", buffer, sizeof(buffer));
						ImGui::PopItemWidth();
						m_SpriteComponentTextureSource = buffer;

						// Set texture source button
						if (ImGui::Button("Set texture", ImVec2(135.0f, 25.0f)))
						{
							if (m_SpriteComponentTextureSource.size())
							{
								std::string filepath(buffer);
								if (AssetsManager::Get().SpriteSheetExists(filepath))
								{
									// If sprite sheet exists then use it
									auto& spriteSheet = AssetsManager::Get().GetSpriteSheet(filepath);
									sprite = CreateShared<Sprite>(spriteSheet);
								}
								else
								{
									// Otherwise use full texture
									auto& texture = AssetsManager::Get().GetTexture(filepath);
									if (texture)
										sprite = CreateShared<Sprite>(texture);
								}
								m_SpriteComponentTextureSource.clear();
							}
						}

						ImGui::SameLine();
						if (ImGui::Button("Remove texture", ImVec2(136.0f, 25.0f)) && sprite)
						{
							sprite.reset();
							m_SpriteComponentTextureSource.clear();
						}

						// Check if texture is spritesheet
						auto& spriteSheet = sprite->m_SpriteSheet;
						if (sprite && spriteSheet)
						{
							int tileX = (int)sprite->m_PosX, tileY = (int)sprite->m_PosY;
							auto& [maxX, maxY] = spriteSheet->GetMaxTilesCount();

							ImGui::Dummy(ImVec2(0.0f, 10.0f));
							ImGui::Text("Spritesheet tile coords:");
							ImGui::Dummy(ImVec2(0.0f, 4.0f));
							ImGui::Columns(2);
							ImGui::SetColumnWidth(0, 75.0f);
							ImGui::Text("Tile X");
							ImGui::NextColumn();

							// Tile coords X position field
							if (ImGui::InputInt("##TPX", &tileX, 1, 1) && tileX != sprite->m_PosX)
								sprite->SetTile((uint32_t)((tileX + maxX) % maxX), sprite->m_PosY);

							ImGui::Columns(1);
							ImGui::Columns(2);
							ImGui::SetColumnWidth(0, 75.0f);
							ImGui::Text("Tile Y");
							ImGui::NextColumn();
								
							// Tile coords Y position field
							if (ImGui::InputInt("##TPY", &tileY, 1, 1) && tileY != sprite->m_PosY)
								sprite->SetTile(sprite->m_PosX, (uint32_t)((tileY + maxY) % maxY));

							ImGui::Columns(1);
						}

						// Draw color control
						ImGui::Dummy(ImVec2(0.0f, 10.0f));
						ImGui::Text("Tint color:");
						ImGui::SameLine();
						ImGui::ColorEdit4("##Color", glm::value_ptr(component.Color));
					});
				}
			}
			ImGui::End();
		}
	}

	template<typename T>
	void Inspector::DrawComponentUI(const std::string& name, std::function<void(T&)> drawContentFunction)
	{
		T& component = m_SelectedEntity.GetComponent<T>();

		ImGuiTreeNodeFlags treeNodeFlags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanAvailWidth
			| ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_FramePadding;
		
		bool expanded = ImGui::TreeNodeEx((void*)typeid(T).hash_code(), treeNodeFlags, name.c_str());

		if (expanded)
		{
			ImGui::Dummy(ImVec2(0.0f, 3.0f));
			drawContentFunction(component);
			ImGui::TreePop();
		}

		ImGui::Dummy(ImVec2(0.0f, 3.0f));
	}

}