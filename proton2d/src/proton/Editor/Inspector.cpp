#include "pch.h"
#include "proton/Editor/Inspector.h"
#include "proton/Editor/EditorOverlay.h"
#include "proton/Scene/ScriptFactory.h"
#include "proton/Assets/AssetsManager.h"
#include "proton/Scene/Components.h"
#include "proton/Core/Utils.h"
#include "proton/Scene/EntityScript.h"

#include <imgui.h>
#include <glm/gtc/type_ptr.hpp>

#define ADD_COMPONENT_POPUP_MENU_ITEM(component) \
	if (!m_SelectedEntity.HasComponent<component>() && ImGui::MenuItem(#component)) \
		m_SelectedEntity.AddComponent<component>(); 

namespace proton {

	void Inspector::OnImGuiRender()
	{
		ImGui::Begin("Inspector");
		if (m_ActiveScene)
		{
			if (m_SelectedEntity)
			{
				// *********************
				// Add component
				// *********************

				if (ImGui::Button("Add component", { 165.0f, 25.0f }))
					ImGui::OpenPopup("add_component_popup");

				if (ImGui::BeginPopup("add_component_popup"))
				{
					ADD_COMPONENT_POPUP_MENU_ITEM(TransformComponent)
					ADD_COMPONENT_POPUP_MENU_ITEM(SpriteComponent)
					ADD_COMPONENT_POPUP_MENU_ITEM(TilemapSpriteComponent)
					ADD_COMPONENT_POPUP_MENU_ITEM(RigidBodyComponent)
					ADD_COMPONENT_POPUP_MENU_ITEM(BoxColliderComponent)

					if (ImGui::BeginMenu("Script"))
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

				// *********************
				// Destroy entity
				// *********************

				ImGui::SameLine();
				if (ImGui::Button("Destroy entity", { 165.0f, 25.0f }))
				{
					m_SelectedEntity.Destroy();
				}
				ImGui::Dummy({ 0.0f, 5.0f });

				// *********************
				// Entity components
				// *********************

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

				// ****************************
				// Transform Component UI
				// ****************************
				if (m_SelectedEntity.HasComponent<TransformComponent>())
				{
					DrawComponentUI<TransformComponent>("Transform", [](auto& component)
					{
						// Draw position control
						ImGui::Columns(2); ImGui::SetColumnWidth(0, 75.0f);
						ImGui::Text("Position");
						ImGui::NextColumn();
						ImGui::PushItemWidth(75.0f);
						ImGui::DragFloat("##P_X", &component.Position.x, 0.01f, 0.0f, 0.0f, "%.3f");
						ImGui::SameLine();
						ImGui::PushItemWidth(75.0f);
						ImGui::DragFloat("##P_Y", &component.Position.y, 0.01f, 0.0f, 0.0f, " %.3f");
						ImGui::SameLine();
						ImGui::PushItemWidth(75.0f);
						ImGui::DragFloat("##P_Z", &component.Position.z, 0.0001f, 0.0f, 0.0f, "%.3f");
						ImGui::Columns(1);

						// Draw Scale control
						ImGui::Columns(2);
						ImGui::SetColumnWidth(0, 75.0f);
						ImGui::Text("Scale");
						ImGui::NextColumn();
						ImGui::PushItemWidth(75.0f);
						ImGui::DragFloat("##S_X", &component.Scale.x, 0.01f, 0.0f, 0.0f, "%.3f");
						ImGui::SameLine();
						ImGui::PushItemWidth(75.0f);
						ImGui::DragFloat("##S_Y", &component.Scale.y, 0.01f, 0.0f, 0.0f, "%.3f");
						ImGui::Columns(1);

						// Draw Rotation control
						ImGui::Columns(2);
						ImGui::SetColumnWidth(0, 75.0f);
						ImGui::Text("Rotation");
						ImGui::NextColumn();
						ImGui::PushItemWidth(75.0f);
						ImGui::DragFloat("##R", &component.Rotation, 0.2f, 0.0f, 0.0f, "%.3f");

						ImGui::Columns(1);
					});
				}

				// ****************************
				// Sprite Component UI
				// ****************************
				if (m_SelectedEntity.HasComponent<SpriteComponent>())
				{
					DrawComponentUI<SpriteComponent>("Sprite", [&](auto& component)
					{
						Shared<Sprite>& sprite = component.Sprite;

						std::string textureFilename = sprite ? sprite->GetTexture()->GetPath() : "Fill color";

						// Select texture
						ImGui::Text("Texture:");
						if (ImGui::BeginCombo("##sprite_comp_select_texture", textureFilename.c_str()))
						{
							if (ImGui::Selectable("Fill color"))
								sprite = nullptr;

							for (auto& kv : AssetsManager::GetTextures())
							{
								bool isSelected = kv.first == textureFilename;

								if (ImGui::Selectable(kv.first.c_str(), isSelected))
								{
									if (AssetsManager::SpriteSheetExists(kv.first))
									{
										auto& spriteSheet = AssetsManager::GetSpriteSheet(kv.first);
										sprite = CreateShared<Sprite>(spriteSheet);
									}
									else
									{
										auto& texture = AssetsManager::GetTexture(kv.first);
										if (texture)
											sprite = CreateShared<Sprite>(texture);
									}
								}

								if (isSelected)
									ImGui::SetItemDefaultFocus();
							}
							ImGui::EndCombo();
						}
						ImGui::Dummy({ 0.0f, 10.0f });

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
									if (ImGui::Selectable(filterModes[i], isSelected) && filterMode != i)
									{
										sprite->GetTexture()->SetFilterMode((TextureFilterMode)i);
										filterMode = i;
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

				// ****************************
				// TilemapSpriteComponent UI
				// ****************************
				if (m_SelectedEntity.HasComponent<TilemapSpriteComponent>())
				{
					DrawComponentUI<TilemapSpriteComponent>("Tilemap sprite", [&](auto& component)
						{
							auto& spritesheet = component.TilemapSprite.m_Spritesheet;
							auto& tilemap = component.TilemapSprite;
							std::string filename = spritesheet ? spritesheet->GetTexture()->GetPath() : "Select...";

							// Select spritesheet
							ImGui::Text("Spritesheet:");
							if (ImGui::BeginCombo("##tilemap_comp_select_spritesheet", filename.c_str()))
							{
								for (auto& kv : AssetsManager::GetSpritesheets())
								{
									bool isSelected = filename == kv.first;

									if (ImGui::Selectable(kv.first.c_str(), isSelected))
									{
										spritesheet = kv.second;
										tilemap.GenerateTilemapBlock();
									}

									if (isSelected)
										ImGui::SetItemDefaultFocus();
								}
								ImGui::EndCombo();
							}
							ImGui::Dummy({ 10.0f, 0.0f });

							// Width and height controls
							if (ImGui::InputInt("Width", (int*)&tilemap.m_Width))
							{
								tilemap.SetSize(std::max(*(int*)&tilemap.m_Width, 1), tilemap.m_Height);
								tilemap.GenerateTilemapBlock();
							}
							if (ImGui::InputInt("Height", (int*)&tilemap.m_Height))
							{
								tilemap.SetSize(tilemap.m_Width, std::max(*(int*)&tilemap.m_Height, 1));
								tilemap.GenerateTilemapBlock();
							}

							ImGui::Dummy({ 10.0f, 0.0f });

							bool left        = tilemap.m_BlockBorders & 1;
							bool right       = tilemap.m_BlockBorders & 2;
							bool top         = tilemap.m_BlockBorders & 4;
							bool bottom      = tilemap.m_BlockBorders & 8;
							bool topLeft     = tilemap.m_BlockBorders & 16;
							bool topRight    = tilemap.m_BlockBorders & 32;
							bool bottomLeft  = tilemap.m_BlockBorders & 64;
							bool bottomRight = tilemap.m_BlockBorders & 128;

							uint8_t before = component.TilemapSprite.m_BlockBorders;

							// Change edges and corners texture
							ImGui::Text("Toggle edges texture: ");
							ImGui::Checkbox("##tb_top_left_corner", &topLeft);
							ImGui::SameLine();
							ImGui::Checkbox("##tb_top_edge", &top);
							ImGui::SameLine();
							ImGui::Checkbox("##tb_top_right_corner", &topRight);
							
							ImGui::Checkbox("##tb_left_edge", &left);
							ImGui::SameLine(); ImGui::Dummy({ 24.0f, 0.0f }); ImGui::SameLine();
							ImGui::Checkbox("##tb_right_edge", &right);

							ImGui::Checkbox("##tb_bottom_left_corner", &bottomLeft);
							ImGui::SameLine();
							ImGui::Checkbox("##tb_bottom_edge", &bottom);
							ImGui::SameLine(); 
							ImGui::Checkbox("##tb_bottom_right_corner", &bottomRight);

							tilemap.m_BlockBorders = left * 1 + right * 2 + top * 4 + bottom * 8
								+ topLeft * 16 + topRight * 32 + bottomLeft * 64 + bottomRight * 128;

							ImGui::Dummy({ 0, 3.0f });

							if (before != tilemap.m_BlockBorders)
								tilemap.GenerateTilemapBlock();

							// Tint color control
							ImGui::Text("Tint color:");
							ImGui::Dummy({ 0.0f, 3.0f });
							ImGui::PushItemWidth(260.0f);
							ImGui::ColorEdit4("##Color", glm::value_ptr(component.Color), ImGuiColorEditFlags_AlphaBar);
							ImGui::PopItemWidth();
							ImGui::Dummy({ 0.0f, 10.0f });
					});
				}

				// ****************************
				// RigidBodyComponent UI
				// ****************************
				if (m_SelectedEntity.HasComponent<RigidBodyComponent>())
				{
					DrawComponentUI<RigidBodyComponent>("RigidBody", [](auto& component)
					{
						std::string bodyType = "Static";
						if (component.Type == b2_dynamicBody)
							bodyType = "Dynamic";
						else if (component.Type == b2_kinematicBody)
							bodyType = "Kinematic";

						if (ImGui::BeginCombo("Body type", bodyType.c_str()))
						{
							if (ImGui::Selectable("Static"))
								component.Type = b2_staticBody;
							else if (ImGui::Selectable("Dynamic"))
								component.Type = b2_dynamicBody;
							else if (ImGui::Selectable("Kinematic"))
								component.Type = b2_kinematicBody;

							ImGui::EndCombo();
						}

						ImGui::Dummy({ 0.0f, 3.0f });
						ImGui::Checkbox("Fixed rotation", &component.FixedRotation);
					});
				}
				// ****************************
				// BoxColliderComponent UI
				// ****************************
				if (m_SelectedEntity.HasComponent<BoxColliderComponent>())
				{
					DrawComponentUI<BoxColliderComponent>("BoxCollider", [](auto& component)
					{
						ImGui::Dummy({ 0.0f, 5.0f });
						ImGui::DragFloat2("Size", glm::value_ptr(component.Size), 0.01f);
						ImGui::DragFloat2("Offset", glm::value_ptr(component.Offset), 0.01f);
						ImGui::DragFloat("Friction", &component.Friction, 0.01f);
						ImGui::DragFloat("Restitution", &component.Restitution, 0.01f);
						ImGui::DragFloat("RestitutionThreshold", &component.RestitutionThreshold, 0.01f);
						ImGui::DragFloat("Density", &component.Density, 0.01f);
						ImGui::Checkbox("IsSensor", &component.IsSensor);
					});
				}

				// Script Component UI
				if (m_SelectedEntity.HasComponent<ScriptComponent>())
				{
					DrawComponentUI<ScriptComponent>("Script", [&](auto& component)
					{
						for (auto& [scriptClassName, scriptData] : component.Scripts)
						{
							ImGui::Text(scriptClassName.c_str());
							
							if (!scriptData.ScriptInstance)
								continue;

							for (auto& [fieldName, fieldData] : scriptData.ScriptInstance->m_ScriptFields)
							{
								if (fieldData.Type == ScriptFieldType::Float)
									ImGui::DragFloat(fieldName.c_str(), (float*)fieldData.InstanceFieldValue, 0.01f);
							}
							ImGui::Dummy({ 0.0f, 10.0f });
						}
					});
				}
			}
			else
			{
				// *********************
				// Scene proporties
				// *********************
				ImGui::Text("Scene proporties:");
				ImGui::Dummy({ 0.0f, 7.0f });

				strcpy_s(m_SceneNameBuffer, m_ActiveScene->m_SceneName.c_str());
				if (ImGui::InputText("Scene name", m_SceneNameBuffer, 256))
					m_ActiveScene->m_SceneName = m_SceneNameBuffer;

				ImGui::DragFloat("World garavity", &m_ActiveScene->m_WorldGravity, 0.1f);
				ImGui::Checkbox("Enable physics", &m_ActiveScene->m_EnablePhysics);
			}
		}

		ImGui::End();
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
	}

}