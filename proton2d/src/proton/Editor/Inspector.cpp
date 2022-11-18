#include "pch.h"
#include "proton/Editor/Inspector.h"
#include "proton/Assets/ScriptFactory.h"
#include "proton/Assets/AssetsManager.h"
#include "proton/Entity/Components.h"

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
				////////////////////////////
				// Add component
				////////////////////////////

				if (ImGui::Button("Add component", { 165.0f, 25.0f }))
					ImGui::OpenPopup("add_component_popup");

				if (ImGui::BeginPopup("add_component_popup"))
				{
					if (!m_SelectedEntity.HasComponent<TransformComponent>())
					{
						if (ImGui::MenuItem("Transform component"))
							m_SelectedEntity.AddComponent<TransformComponent>();
					}

					if (!m_SelectedEntity.HasComponent<SpriteComponent>())
					{
						if (ImGui::MenuItem("Sprite component"))
							m_SelectedEntity.AddComponent<SpriteComponent>();
					}

					if (ImGui::BeginMenu("Script component"))
					{
						for (auto& [scriptName, addScriptFunction] : ScriptFactory::GetScripts())
						{
							if (ImGui::MenuItem(scriptName.c_str()))
								addScriptFunction(m_SelectedEntity);
						}
						ImGui::EndMenu();
					}

					ImGui::EndPopup();
				}

				////////////////////////////
				// Destroy entity
				////////////////////////////

				ImGui::SameLine();
				if (ImGui::Button("Destroy entity", { 165.0f, 25.0f }))
				{
					m_SelectedEntity.Destroy();
				}

				////////////////////////////
				// Entity components
				////////////////////////////

				ImGui::Dummy({ 0.0f, 5.0f });
				ImGui::Text("Attached components:");
				ImGui::Dummy({ 0.0f, 5.0f });

				// Tag Component UI
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

				// Transform Component UI
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
						ImGui::DragFloat("##P_Z", &component.Position.z, 0.0001f, 0.0f, 0.0f, "%.2f");
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
						ImGui::DragFloat("##R", &component.Rotation, 0.2f, 0.0f, 0.0f, "%.2f");

						ImGui::Columns(1);
					});
				}

				// Sprite Component UI
				if (m_SelectedEntity.HasComponent<SpriteComponent>())
				{
					DrawComponentUI<SpriteComponent>("Sprite", [&](auto& component)
					{
						Shared<Sprite>& sprite = component.Sprite;
						std::string& textureSource = m_SpriteComponentTextureSource;
						
						// If component has sprite assigned then store it's path
						if (sprite && !textureSource.size())
							textureSource = sprite->GetTexture()->GetPath();

						// Texutre source input
						char buffer[256];
						strcpy_s(buffer, sizeof(buffer), textureSource.c_str());
						ImGui::Text("Texture source:");
						ImGui::Dummy({ 0.0f, 3.0f });
						ImGui::PushItemWidth(280.0f);
						ImGui::InputText("##Texture_Source", buffer, sizeof(buffer));
						ImGui::PopItemWidth();
						textureSource = buffer;

						// Set texture source button
						if (ImGui::Button("Set texture", { 135.0f, 25.0f }))
						{
							if (textureSource.size())
							{
								if (AssetsManager::SpriteSheetExists(textureSource))
								{
									// If sprite sheet exists then use it
									auto& spriteSheet = AssetsManager::GetSpriteSheet(textureSource);
									sprite = CreateShared<Sprite>(spriteSheet);
								}
								else
								{
									// Otherwise use full texture
									auto& texture = AssetsManager::GetTexture(textureSource);
									if (texture)
										sprite = CreateShared<Sprite>(texture);
								}
								textureSource.clear();
							}
						}

						// Remove texture button
						if (sprite)
						{
							ImGui::SameLine();
							if (ImGui::Button("Remove texture", ImVec2(136.0f, 25.0f)) && sprite)
							{
								sprite.reset();
								textureSource.clear();
							}
							ImGui::Dummy({ 0.0f, 10.0f });
						}

						// Tint color control
						ImGui::Text("Tint color:");
						ImGui::Dummy({ 0.0f, 3.0f });
						ImGui::PushItemWidth(260.0f);
						ImGui::ColorEdit4("##Color", glm::value_ptr(component.Color), ImGuiColorEditFlags_AlphaBar);
						ImGui::PopItemWidth();
						ImGui::Dummy({ 0.0f, 10.0f });

						// Texture filter mode
						if (sprite)
						{
							ImGui::Text("Filter mode:"); ImGui::SameLine();
							uint32_t filterMode = (uint32_t)sprite->GetTexture()->GetFilterMode();
							const char* filterModes[] = { "Nearest", "Linear" };

							if (ImGui::BeginCombo("##Texture_Filter", filterModes[filterMode]))
							{
								for (uint32_t i = 0; i < 2; i++)
								{
									const bool isSelected = (filterMode == i);
									if (ImGui::Selectable(filterModes[i], isSelected))
									{
										if (filterMode != i)
										{
											sprite->GetTexture()->SetFilterMode((TextureFilterMode)i);
											filterMode = i;
										}
									}

									if (isSelected)
										ImGui::SetItemDefaultFocus();
								}
								ImGui::EndCombo();
							}
							ImGui::Dummy({ 0.0f, 10.0f });
							ImGui::Text("Sprite flip: ");
							ImGui::SameLine();
							ImGui::Checkbox("X##Flip", &sprite->m_FlipX);
							ImGui::SameLine();
							ImGui::Checkbox("Y##Flip", &sprite->m_FlipY);
							ImGui::Dummy({ 0.0f, 10.0f });
						}

						// Check if texture is spritesheet
						if (sprite && sprite->m_SpriteSheet)
						{
							int tileX = (int)sprite->m_PosX, tileY = (int)sprite->m_PosY;
							auto& [maxX, maxY] = sprite->m_SpriteSheet->GetMaxTilesCount();

							ImGui::Text("Spritesheet tile coordinates:");
							ImGui::Dummy({ 0.0f, 4.0f });
							ImGui::Columns(2);
							ImGui::SetColumnWidth(0, 35.0f);
							ImGui::Text("X");
							ImGui::NextColumn();

							// Tile coords X position field
							if (ImGui::InputInt("##Tile_Pos_X", &tileX, 1, 1) && tileX != sprite->m_PosX)
								sprite->SetTile((uint32_t)((tileX + maxX) % maxX), sprite->m_PosY);

							ImGui::Columns(1);
							ImGui::Columns(2);
							ImGui::SetColumnWidth(0, 35.0f);
							ImGui::Text("Y");
							ImGui::NextColumn();
								
							// Tile coords Y position field
							if (ImGui::InputInt("##Tile_Pos_Y", &tileY, 1, 1) && tileY != sprite->m_PosY)
								sprite->SetTile(0, (uint32_t)((tileY + maxY) % maxY));

							ImGui::Columns(1);
							ImGui::Dummy({ 0.0f, 10.0f });
						}
					});
				}

				// Script Component UI
				if (m_SelectedEntity.HasComponent<ScriptComponent>())
				{
					DrawComponentUI<ScriptComponent>("Script", [&](auto& component)
					{
						for (auto& [scriptName, scriptData] : component.Scripts)
							ImGui::Text(scriptName.c_str());
					});
				}
			}
			ImGui::End();
		}
	}

	template<typename T>
	void Inspector::DrawComponentUI(const std::string& name, const std::function<void(T&)>& drawContentFunction)
	{
		T& component = m_SelectedEntity.GetComponent<T>();

		ImGuiTreeNodeFlags treeNodeFlags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanAvailWidth
			| ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_FramePadding;
		
		bool opened = ImGui::TreeNodeEx((void*)typeid(T).hash_code(), treeNodeFlags, name.c_str());

		bool removeComponent = false;
		if (!std::is_same<T, TagComponent>::value && !std::is_same<T, TransformComponent>::value)
		{
			ImGui::SameLine(ImGui::GetWindowWidth() - 90.0f);
			removeComponent = ImGui::Button(("Remove##" + name).c_str());
		}

		if (opened)
		{
			ImGui::Dummy({ 0.0f, 3.0f });
			drawContentFunction(component);
			ImGui::TreePop();
		}

		if (removeComponent)
		{
			m_SelectedEntity.RemoveComponent<T>();
		}

		ImGui::Dummy({ 0.0f, 3.0f });
	}

	void Inspector::SetSelectionContext(Entity entity)
	{
		m_SelectedEntity = entity;
		m_SpriteComponentTextureSource.clear();
	}

}