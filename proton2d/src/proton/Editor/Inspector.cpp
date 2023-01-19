#include "pch.h"
#include "proton/Editor/Inspector.h"
#include "proton/Editor/EditorOverlay.h"
#include "proton/Scene/ScriptFactory.h"
#include "proton/Assets/AssetsManager.h"
#include "proton/Scene/Components.h"
#include "proton/Core/Utils.h"
#include "proton/Scene/EntityScript.h"
#include "proton/Scene/PrefabManager.h"
#include "proton/Core/Application.h"
#include "proton/Graphics/Renderer.h"

#include <imgui.h>
#include <glm/gtc/type_ptr.hpp>

#define ADD_COMPONENT_POPUP_MENU_ITEM(component) \
	if (!m_SelectedEntity.HasComponent<component>() && ImGui::MenuItem(#component)) \
		m_SelectedEntity.AddComponent<component>()

namespace proton {

	static std::string HexUUID(UUID uuid)
	{
		std::stringstream stream;
		stream << std::hex << (uint64_t)uuid;
		return stream.str();
	}

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
					ADD_COMPONENT_POPUP_MENU_ITEM(TransformComponent);
					ADD_COMPONENT_POPUP_MENU_ITEM(SpriteComponent);
					ADD_COMPONENT_POPUP_MENU_ITEM(NineSliceSpriteComponent);
					ADD_COMPONENT_POPUP_MENU_ITEM(CameraComponent);
					ADD_COMPONENT_POPUP_MENU_ITEM(RigidbodyComponent);
					ADD_COMPONENT_POPUP_MENU_ITEM(BoxColliderComponent);

					ImGui::Separator();
					if (ImGui::BeginMenu("Script"))
					{
						for (auto& [scriptName, addScriptFunction] : ScriptFactory::Get().m_ScriptRegistry)
						{
							if (ImGui::MenuItem(scriptName.c_str()))
								addScriptFunction(m_SelectedEntity);
						}
						ImGui::EndMenu();
					}

					ImGui::EndPopup();
				}

				// *********************
				// Destroy entity, Entity ID, Create prefab
				// *********************

				ImGui::SameLine();
				if (ImGui::Button("Destroy entity", { 165.0f, 25.0f }))
				{
					m_SelectedEntity.Destroy();
					ImGui::End();
					return;
				}
				ImGui::Dummy({ 0.0f, 5.0f });

				ImGui::Dummy({ 3,0 }); ImGui::SameLine();
				std::string uuid = "UUID: " + HexUUID(m_SelectedEntity.GetUUID());
				ImGui::Text(uuid.c_str());
				ImGui::SameLine(ImGui::GetWindowWidth() - 140.0f);

				ImGui::SetCursorPosX(ImGui::GetWindowWidth() / 2 + ImGui::CalcTextSize(uuid.c_str()).x - 140);
				if (ImGui::Button("Create prefab", { 120.0f, 25.0f }))
				{
					PrefabManager::SaveAsPrefab(m_SelectedEntity);
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
					DrawComponentUI<TransformComponent>("Transform", [&](auto& component)
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
						if (ImGui::DragFloat("##S_X", &component.Scale.x, 0.01f, 0.0f, 0.0f, "%.3f"))
						{
							if (m_SelectedEntity.HasComponent<NineSliceSpriteComponent>())
							{
								auto& nsc = m_SelectedEntity.GetComponent<NineSliceSpriteComponent>();
								nsc.NineSliceSprite.Refresh();
							}
						}
						ImGui::SameLine();
						ImGui::PushItemWidth(75.0f);
						if(ImGui::DragFloat("##S_Y", &component.Scale.y, 0.01f, 0.0f, 0.0f, "%.3f"))
						{
							if (m_SelectedEntity.HasComponent<NineSliceSpriteComponent>())
							{
								auto& nsc = m_SelectedEntity.GetComponent<NineSliceSpriteComponent>();
								nsc.NineSliceSprite.Refresh();
							}
						}
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
						ImGui::PushItemWidth(200.0f);
						if (ImGui::BeginCombo("##sprite_comp_select_texture", textureFilename.c_str()))
						{
							if (ImGui::Selectable("Fill color"))
								sprite = nullptr;

							// Spritesheets
							for (auto& kv : AssetsManager::s_Instance->m_SpritesheetList)
							{
								bool isSelected = kv.first == textureFilename;
								ImGui::PushStyleColor(ImGuiCol_Text, { 0.0f, 0.9f, 0.3f, 1.0f });
								if (ImGui::Selectable(kv.first.c_str(), isSelected))
								{
									auto& spritesheet = AssetsManager::GetSpriteSheet(kv.first);
									if (spritesheet)
										sprite = CreateShared<Sprite>(spritesheet);
								}
								ImGui::PopStyleColor();
								if (isSelected)
									ImGui::SetItemDefaultFocus();
							}
							// Textures
							for (auto& path : AssetsManager::s_Instance->m_TexturesFilepathList)
							{
								bool isSelected = path == textureFilename;
								ImGui::PushStyleColor(ImGuiCol_Text, { 0.9f, 0.8f, 0.1f, 1.0f });
								if (ImGui::Selectable(path.c_str(), isSelected))
								{
									auto& texture = AssetsManager::GetTexture(path);
									if (texture)
										sprite = CreateShared<Sprite>(texture);
								}
								ImGui::PopStyleColor();
								if (isSelected)
									ImGui::SetItemDefaultFocus();
							}
							ImGui::EndCombo();
						}
						if (ImGui::IsItemClicked())
							AssetsManager::ReloadAssetsList();
						ImGui::PopItemWidth();

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
							ImGui::Text("Mirror flip: ");
							ImGui::SameLine();
							ImGui::Checkbox("X##Flip", &sprite->m_MirrorFlipX);
							ImGui::SameLine();
							ImGui::Checkbox("Y##Flip", &sprite->m_MirrorFlipY);
							ImGui::Dummy({ 0.0f, 10.0f });

							// Tiling factor
							ImGui::DragFloat("Tiling factor", &component.TilingFactor, 0.1f);
							ImGui::Dummy({ 0.0f, 10.0f });
						}

						// Check if texture is spritesheet
						if (sprite && sprite->m_SpriteSheet)
						{
							int tileX = (int)sprite->m_TilePos.x, tileY = (int)sprite->m_TilePos.y;
							const auto& count = sprite->m_SpriteSheet->GetTileCount();

							ImGui::Text("Spritesheet tile coordinates:");
							ImGui::Dummy({ 0.0f, 4.0f });
							ImGui::Columns(2);
							ImGui::SetColumnWidth(0, 35.0f);
							ImGui::Text("X");
							ImGui::NextColumn();

							// Tile coords X position field
							if (ImGui::InputInt("##Tile_Pos_X", &tileX, 1, 1) && tileX != sprite->m_TilePos.x)
								sprite->SetTile((uint32_t)((tileX + count.x) % count.x), sprite->m_TilePos.y);

							ImGui::Columns(1);
							ImGui::Columns(2);
							ImGui::SetColumnWidth(0, 35.0f);
							ImGui::Text("Y");
							ImGui::NextColumn();
								
							// Tile coords Y position field
							if (ImGui::InputInt("##Tile_Pos_Y", &tileY, 1, 1) && tileY != sprite->m_TilePos.y)
								sprite->SetTile(0, (uint32_t)((tileY + count.y) % count.y));

							ImGui::Columns(1);
							ImGui::Dummy({ 0.0f, 10.0f });
						}
					});
				}

				// ****************************
				// NineSliceSpriteComponent UI
				// ****************************
				if (m_SelectedEntity.HasComponent<NineSliceSpriteComponent>())
				{
					DrawComponentUI<NineSliceSpriteComponent>("Nine Slice Sprite", [&](auto& component)
						{
							auto& spritesheet = component.NineSliceSprite.m_Spritesheet;
							auto& sprite = component.NineSliceSprite;
							std::string filename = spritesheet ? spritesheet->GetTexture()->GetPath() : "Select...";

							// Select spritesheet
							ImGui::Text("Spritesheet:");
							if (ImGui::BeginCombo("##nine_slice_select_spritesheet", filename.c_str()))
							{
								for (auto& kv : AssetsManager::s_Instance->m_SpritesheetList)
								{
									bool isSelected = filename == kv.first;

									if (ImGui::Selectable(kv.first.c_str(), isSelected))
									{
										spritesheet = AssetsManager::GetSpriteSheet(kv.first);
										sprite.SetSpritesheet(spritesheet);
									}

									if (isSelected)
										ImGui::SetItemDefaultFocus();
								}
								ImGui::EndCombo();
							}
							if (ImGui::IsItemClicked())
								AssetsManager::ReloadAssetsList();

							ImGui::Dummy({ 0.0f, 5.0f });
							float tileScale = sprite.m_TileScale;
							if (ImGui::DragFloat("Tile scale", &tileScale, 0.001f))
							{
								sprite.SetTileScale(tileScale);
							}
							ImGui::DragInt2("Spritesheet offset", (int*)glm::value_ptr(sprite.m_PositionOffset));
							ImGui::Dummy({ 0.0f, 5.0f });

							bool left        = sprite.m_BlockBorders & 1;
							bool right       = sprite.m_BlockBorders & 2;
							bool top         = sprite.m_BlockBorders & 4;
							bool bottom      = sprite.m_BlockBorders & 8;
							bool topLeft     = sprite.m_BlockBorders & 16;
							bool topRight    = sprite.m_BlockBorders & 32;
							bool bottomLeft  = sprite.m_BlockBorders & 64;
							bool bottomRight = sprite.m_BlockBorders & 128;

							// Toggle texture borders and corners
							ImGui::Text("Toggle texture borders:");
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

							sprite.SetBlockBorders(left * 1 + right * 2 + top * 4 + bottom * 8
								+ topLeft * 16 + topRight * 32 + bottomLeft * 64 + bottomRight * 128);

							ImGui::Dummy({ 0, 3.0f });

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
				// CameraComponent UI
				// ****************************
				if (m_SelectedEntity.HasComponent<CameraComponent>())
				{
					DrawComponentUI<CameraComponent>("Camera", [&](auto& component)
						{
							bool isPrimary = m_ActiveScene->m_PrimaryCameraEntity == m_SelectedEntity.m_Handle;
							if (ImGui::Checkbox("Set as primary", &isPrimary))
							{
								if (isPrimary)
									m_ActiveScene->SetPrimaryCameraEntity(m_SelectedEntity);
								else
									m_ActiveScene->SetPrimaryCameraEntity(Entity{});
							}

							float zoom = component.Camera->GetZoomLevel();
							if (ImGui::DragFloat("Zoom level", &zoom, 0.01f))
								component.Camera->SetZoomLevel(zoom);
							ImGui::DragFloat2("Position offset", glm::value_ptr(component.PositionOffset), 0.01f);
						});
				}

				// ****************************
				// RigidbodyComponent UI
				// ****************************
				if (m_SelectedEntity.HasComponent<RigidbodyComponent>())
				{
					DrawComponentUI<RigidbodyComponent>("Rigidbody", [](auto& component)
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
						ImGui::DragFloat("Friction", &component.Material.Friction, 0.01f);
						ImGui::DragFloat("Restitution", &component.Material.Restitution, 0.01f);
						ImGui::DragFloat("RestitutionThreshold", &component.Material.RestitutionThreshold, 0.01f);
						ImGui::DragFloat("Density", &component.Material.Density, 0.01f);
						ImGui::Checkbox("IsSensor", &component.IsSensor);
					});
				}

				// ****************************
				// Script Component UI
				// ****************************
				if (m_SelectedEntity.HasComponent<ScriptComponent>())
				{
					auto& component = m_SelectedEntity.GetComponent<ScriptComponent>();
					for (auto& [scriptClassName, scriptInstance] : component.Scripts)
					{
						if (!scriptInstance)
							continue;
							
						ImGuiTreeNodeFlags treeNodeFlags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanAvailWidth
							| ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_FramePadding;

						std::string title = scriptClassName + " (Script)";
						bool opened = ImGui::TreeNodeEx(scriptClassName.c_str(), treeNodeFlags, title.c_str());

						ImGui::SameLine(ImGui::GetWindowWidth() - 90.0f);
						bool removeScript = ImGui::Button(("Remove##" + scriptClassName).c_str());

						if (opened)
						{
							ImGui::Dummy({ 0.0f, 3.0f });
							for (auto& [fieldName, fieldData] : scriptInstance->m_ScriptFields)
							{
								switch (fieldData.Type)
								{
								case ScriptFieldType::Float:
									ImGui::DragFloat(fieldName.c_str(), (float*)fieldData.InstanceFieldValue, 0.01f);
									break;
								case ScriptFieldType::Float2:
									ImGui::DragFloat2(fieldName.c_str(), (float*)fieldData.InstanceFieldValue, 0.01f);
									break;
								case ScriptFieldType::Float3:
									ImGui::DragFloat3(fieldName.c_str(), (float*)fieldData.InstanceFieldValue, 0.01f);
									break;
								case ScriptFieldType::Float4:
									ImGui::DragFloat4(fieldName.c_str(), (float*)fieldData.InstanceFieldValue, 0.01f);
									break;

								case ScriptFieldType::Int:
									ImGui::DragInt(fieldName.c_str(), (int*)fieldData.InstanceFieldValue);
									break;
								case ScriptFieldType::Int2:
									ImGui::DragInt2(fieldName.c_str(), (int*)fieldData.InstanceFieldValue);
									break;
								case ScriptFieldType::Int3:
									ImGui::DragInt3(fieldName.c_str(), (int*)fieldData.InstanceFieldValue);
									break;
								case ScriptFieldType::Int4:
									ImGui::DragInt4(fieldName.c_str(), (int*)fieldData.InstanceFieldValue);
									break;

								case ScriptFieldType::Bool:
									ImGui::Checkbox(fieldName.c_str(), (bool*)fieldData.InstanceFieldValue);
									break;
								}
							}
							ImGui::TreePop();
						}

						if (removeScript)
							m_SelectedEntity.RemoveScript(scriptClassName);
						ImGui::Dummy({ 0.0f, 10.0f });
					}
				}
			}
			else
			{
				// *********************
				// Scene proporties
				// *********************
				ImGui::Text("Scene proporties");
				ImGui::Separator();
				ImGui::Dummy({ 0.0f, 3.0f });

				ImGui::Text("Scene name");
				strcpy_s(m_SceneNameBuffer, m_ActiveScene->m_SceneName.c_str());
				if (ImGui::InputText("##scene_name", m_SceneNameBuffer, 256))
					m_ActiveScene->m_SceneName = m_SceneNameBuffer;

				ImGui::Dummy({ 0.0f, 5.0f });
				ImGui::Text("Screen clear color");
				if (ImGui::ColorEdit4("##screen_clear_color", glm::value_ptr(m_ActiveScene->m_ClearColor)))
					Renderer::SetClearColor(m_ActiveScene->m_ClearColor);
				ImGui::Dummy({ 0.0f, 5.0f });

				ImGui::Text("Scene physics settings");
				ImGui::Dummy({ 0.0f, 3.0f });
				ImGui::Separator();
				ImGui::Checkbox("Enable physics simulation", &m_ActiveScene->m_EnablePhysics);
				ImGui::Dummy({ 0,5 });
				ImGui::PushItemWidth(100.0f);
				ImGui::DragFloat("World gravity", &m_ActiveScene->m_WorldGravity, 0.1f);
				int* vi = &m_ActiveScene->m_PhysicsVelocityIterations;
				int* pi = &m_ActiveScene->m_PhysicsPositionIterations;
				if (ImGui::DragInt("Velocity iterations", vi))
					*vi = glm::max(*vi, 1);
				if (ImGui::DragInt("Position iterations", pi))
					*pi = glm::max(*pi, 1);
				ImGui::PopItemWidth();
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