#include "ptpch.h"
#ifdef PT_EDITOR
#include "Proton/Editor/Panels/InspectorPanel.h"
#include "Proton/Editor/Panels/SceneViewportPanel.h"
#include "Proton/Editor/EditorLayer.h"

#include "Proton/Core/Application.h"
#include "Proton/Core/GameInstance.h"
#include "Proton/Core/AssetManager.h"
#include "Proton/Graphics/Renderer/Renderer.h"
#include "Proton/Scene/SceneManager.h"
#include "Proton/Scene/PrefabManager.h"
#include "Proton/Scripting/ScriptFactory.h"
#include "Proton/Scripting/GameModeBase.h"
#include "Proton/Scripting/EntityScript.h"
#include "Proton/Physics/PhysicsWorld.h"
#include "Proton/Utils/Utils.h"
#include "Proton/UI/UIText.h"

#include "Proton/Network/NetworkManager.h"

#include <imgui.h>
#include <glm/gtc/type_ptr.hpp>

namespace proton {

	static const std::string s_TexturesPath = "content/textures/";

	static std::string GetFilepathRelative(const std::string& parentDir, const std::string& fullFilepath)
	{
		return fullFilepath.substr(parentDir.size(), fullFilepath.size() - parentDir.size());;
	}

	static std::string UintToHex(uint64_t value) {
		std::stringstream stream;
		stream << std::hex << std::setw(16) << std::setfill('0') << value;
		return stream.str();
	}
	
	static bool DrawTreeNodeRemoveButton(const std::string& id)
	{
		ImGui::PushFont(EditorLayer::GetFontAwesome());
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.45f, 0.45f, 0.45f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.55f, 0.55f, 0.55f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.65f, 0.65f, 0.65f, 1.0f));
		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 100.0f);
		ImGui::SameLine(ImGui::GetContentRegionMax().x - 20.0f);
		bool pressed = ImGui::Button((u8"\uF00D##" + id).c_str());
		ImGui::PopFont();
		ImGui::PopStyleColor(3);
		ImGui::PopStyleVar();
		return pressed;
	}

	void InspectorPanel::OnImGuiRender()
	{
		ImGui::Begin("Inspector");

		Scene* scene = GetActiveScene();
		Entity selectedEntity = GetSelectedEntity();

		if (!scene) {
			ImGui::End();
			return;
		}

		// Draw scene proporties if no entity is selected
		if (!selectedEntity.IsValid())
		{
			DrawSceneProporties();
			ImGui::End();
			return;
		}

		bool isPrefab = selectedEntity.HasComponent<PrefabComponent>();

		char buffer[256];
		strcpy_s(buffer, sizeof(buffer), selectedEntity.GetTag().c_str());
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, { 8, 5 });
		if (ImGui::InputText("##tag", buffer, sizeof(buffer)))
			selectedEntity.GetComponent<TagComponent>().Tag = std::string(buffer);
		ImGui::PopStyleVar();
		ImGui::SameLine();

		// Add component popup
		ImGui::NextColumn();
		ImGui::PushFont(EditorLayer::GetFontAwesome());
		if (ImGui::Button(u8"\uF067", { 42, 28 }))
			ImGui::OpenPopup("Add component");
		ImGui::PopFont();

		if (ImGui::BeginPopup("Add component"))
		{
			#define ADD_COMPONENT_POPUP_MENU_ITEM(component) \
			if (!selectedEntity.HasComponent<component>() && ImGui::MenuItem(#component)) \
				selectedEntity.AddComponent<component>()

			ADD_COMPONENT_POPUP_MENU_ITEM(TransformComponent);
			ADD_COMPONENT_POPUP_MENU_ITEM(NetworkComponent);
			ADD_COMPONENT_POPUP_MENU_ITEM(SpriteComponent);
			ADD_COMPONENT_POPUP_MENU_ITEM(ResizableSpriteComponent);
			ADD_COMPONENT_POPUP_MENU_ITEM(CircleRendererComponent);
			ADD_COMPONENT_POPUP_MENU_ITEM(TextComponent);
			ADD_COMPONENT_POPUP_MENU_ITEM(CameraComponent);
			ADD_COMPONENT_POPUP_MENU_ITEM(RigidbodyComponent);
			ADD_COMPONENT_POPUP_MENU_ITEM(BoxColliderComponent);
			ADD_COMPONENT_POPUP_MENU_ITEM(CircleColliderComponent);

			// Scripts list
			ImGui::Separator();
			if (ImGui::BeginMenu("Script"))
			{
				for (auto& [scriptName, addScriptFunction] : ScriptFactory::Get().m_ScriptRegistry)
				{
					if (selectedEntity.HasComponent<ScriptComponent>())
					{
						auto& component = selectedEntity.GetComponent<ScriptComponent>();
						if (component.Scripts.find(scriptName) != component.Scripts.end())
							continue;
					}
					if (ImGui::MenuItem(scriptName.c_str()))
						addScriptFunction(selectedEntity);
				}
				ImGui::EndMenu();
			}

			ImGui::EndPopup();
		}

		ImGui::SameLine();
		ImGui::PushFont(EditorLayer::GetFontAwesome());
		if (ImGui::Button(u8"\uF141", { 42, 28 }))
		{
			ImGui::OpenPopup("Entity options");
		}
		ImGui::PopFont();

		if (ImGui::BeginPopup("Entity options")) 
		{
			if (ImGui::MenuItem(isPrefab ? "Save Prefab" : "Create Prefab"))
			{
				PrefabManager::SaveEntityAsPrefab(selectedEntity);
			}

			if (isPrefab && ImGui::MenuItem("Detach Prefab"))
			{
				selectedEntity.RemoveComponent<PrefabComponent>();
			}

			ImGui::Separator();
			if (ImGui::MenuItem("Delete Entity"))
			{
				selectedEntity.Destroy();
				EditorLayer::SelectEntity({});
				ImGui::EndPopup();
				ImGui::End();
				return;
			}
			ImGui::EndPopup();
		}
		ImGui::Columns(1);

		ImGui::Dummy({ 1, 0 }); ImGui::SameLine();
		ImGui::Text("UUID: %s", UintToHex(selectedEntity.GetUUID()).c_str());

		ImGui::Dummy({ 0, 5 });

		// ******************************************************
		// Transform Component UI
		// ******************************************************
		if (selectedEntity.HasComponent<TransformComponent>())
		{
			DrawComponentUI<TransformComponent>("Transform", [&](auto& component)
			{
				// Posittion
				ImGui::Columns(2); ImGui::SetColumnWidth(0, 100.0f);
				ImGui::Text("Position");
				ImGui::NextColumn();
				ImGui::PushItemWidth(75.0f);
				ImGui::DragFloat("##P_X", &component.LocalPosition.x, 0.01f, 0.0f, 0.0f, "%.3f");
				ImGui::SameLine();
				ImGui::PushItemWidth(75.0f);
				ImGui::DragFloat("##P_Y", &component.LocalPosition.y, 0.01f, 0.0f, 0.0f, " %.3f");
				ImGui::SameLine();
				ImGui::PushItemWidth(75.0f);
				ImGui::DragFloat("##P_Z", &component.LocalPosition.z, 0.0001f, 0.0f, 0.0f, "%.3f");
				ImGui::Columns(1);

				// Scale
				ImGui::Columns(2);
				ImGui::SetColumnWidth(0, 100.0f);
				ImGui::Text("Scale");
				ImGui::NextColumn();
				ImGui::PushItemWidth(75.0f);
				if (ImGui::DragFloat("##S_X", &component.Scale.x, 0.01f, 0.0f, 0.0f, "%.3f"))
				{
					if (selectedEntity.HasComponent<ResizableSpriteComponent>())
					{
						auto& nsc = selectedEntity.GetComponent<ResizableSpriteComponent>();
						nsc.ResizableSprite.Generate(component.Scale);
					}
				}
				ImGui::SameLine();
				ImGui::PushItemWidth(75.0f);
				if(ImGui::DragFloat("##S_Y", &component.Scale.y, 0.01f, 0.0f, 0.0f, "%.3f"))
				{
					if (selectedEntity.HasComponent<ResizableSpriteComponent>())
					{
						auto& nsc = selectedEntity.GetComponent<ResizableSpriteComponent>();
						nsc.ResizableSprite.Generate(component.Scale);
					}
				}
				ImGui::Columns(1);

				// Rotation 
				ImGui::Columns(2);
				ImGui::SetColumnWidth(0, 100.0f);
				ImGui::Text("Rotation");
				ImGui::NextColumn();
				ImGui::PushItemWidth(75.0f);
				ImGui::DragFloat("##R", &component.Rotation, 0.2f, 0.0f, 0.0f, "%.3f");

				ImGui::Columns(1);
			});
		}

		// ******************************************************
		// Script Component UI
		// ******************************************************
		if (selectedEntity.HasComponent<ScriptComponent>())
		{
			auto& component = selectedEntity.GetComponent<ScriptComponent>();
			for (auto& [scriptClassName, scriptInstance] : component.Scripts)
			{
				if (!scriptInstance)
					continue;

				ImGuiTreeNodeFlags treeNodeFlags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanAvailWidth
					| ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_FramePadding;

				std::string label = scriptClassName + " (Script)";
				bool opened = ImGui::TreeNodeEx(label.c_str(), treeNodeFlags, label.c_str());

				bool removeScript = DrawTreeNodeRemoveButton(scriptClassName);

				if (opened)
				{
					ImGui::Dummy({ 0.0f, 3.0f });
					for (auto& [fieldName, fieldData] : scriptInstance->m_ScriptFields)
					{
						if (!scriptInstance->m_EditorScriptField[fieldName].ShowInEditor)
							continue;

						switch (fieldData.Type)
						{
						case ScriptFieldType::Float:
							ImGui::DragFloat(fieldName.c_str(), (float*)fieldData.ValuePtr, 0.01f);
							break;
						case ScriptFieldType::Float2:
							ImGui::DragFloat2(fieldName.c_str(), (float*)fieldData.ValuePtr, 0.01f);
							break;
						case ScriptFieldType::Float3:
							ImGui::DragFloat3(fieldName.c_str(), (float*)fieldData.ValuePtr, 0.01f);
							break;
						case ScriptFieldType::Float4:
							if (fieldName.find("Color") != fieldName.npos || fieldName.find("color") != fieldName.npos)
								ImGui::ColorEdit4(fieldName.c_str(), (float*)fieldData.ValuePtr);
							else
								ImGui::DragFloat4(fieldName.c_str(), (float*)fieldData.ValuePtr, 0.01f);
							break;

						case ScriptFieldType::Int:
							ImGui::DragInt(fieldName.c_str(), (int*)fieldData.ValuePtr);
							break;
						case ScriptFieldType::Int2:
							ImGui::DragInt2(fieldName.c_str(), (int*)fieldData.ValuePtr);
							break;
						case ScriptFieldType::Int3:
							ImGui::DragInt3(fieldName.c_str(), (int*)fieldData.ValuePtr);
							break;
						case ScriptFieldType::Int4:
							ImGui::DragInt4(fieldName.c_str(), (int*)fieldData.ValuePtr);
							break;

						case ScriptFieldType::Bool:
							ImGui::Checkbox(fieldName.c_str(), (bool*)fieldData.ValuePtr);
							break;

						case ScriptFieldType::String:
						{
							static char buffer[1024];
							strcpy_s(buffer, ((std::string*)fieldData.ValuePtr)->c_str());
							if (ImGui::InputText(fieldName.c_str(), buffer, 1024))
							{
								std::string& strRef = *(std::string*)fieldData.ValuePtr;
								strRef = buffer;
							}
							break;
						}
						}
					}
					scriptInstance->OnImGuiRender();
					ImGui::TreePop();
				}

				if (removeScript)
				{
					selectedEntity.RemoveScript(scriptClassName);
					break;
				}
				ImGui::Dummy({ 0.0f, 5.0f });
			}
		}

		// ******************************************************
		// Sprite Component UI
		// ******************************************************
		if (selectedEntity.HasComponent<SpriteComponent>())
		{
			DrawComponentUI<SpriteComponent>("Sprite", [&](auto& component)
			{
				Sprite& sprite = component.Sprite;
				std::string textureFilename = sprite 
					? GetFilepathRelative(s_TexturesPath, sprite.GetTexture()->GetPath())
					: "Fill color";

				// Select texture
				if (ImGui::BeginCombo("Texture", textureFilename.c_str()))
				{
					if (ImGui::Selectable("Fill Color"))
						sprite.SetTexture(nullptr);

					// Spritesheets
					for (auto& kv : AssetManager::s_Instance->m_SpritesheetList)
					{
						bool isSelected = kv.first == textureFilename;
						ImGui::PushStyleColor(ImGuiCol_Text, { 0.0f, 0.9f, 0.3f, 1.0f });
						if (ImGui::Selectable(kv.first.c_str(), isSelected))
						{
							auto& spritesheet = AssetManager::GetSpritesheet(kv.first);
							if (spritesheet)
							{
								sprite = Sprite(spritesheet);
								auto& scale = selectedEntity.GetTransform().Scale;
								float ratio = sprite.GetAspectRatio();
								if (scale.x / scale.y != ratio)
									scale.x = scale.y * ratio;
							}
						}
						ImGui::PopStyleColor();
						if (isSelected)
							ImGui::SetItemDefaultFocus();
					}
					// Textures
					for (auto& path : AssetManager::s_Instance->m_TexturesFilepathList)
					{
						bool isSelected = path == textureFilename;
						ImGui::PushStyleColor(ImGuiCol_Text, { 0.9f, 0.8f, 0.1f, 1.0f });
						if (ImGui::Selectable(path.c_str(), isSelected))
						{
							auto& texture = AssetManager::GetTexture(path);
							if (texture)
							{
								sprite = Sprite(texture);
								auto& scale = selectedEntity.GetTransform().Scale;
								float ratio = sprite.GetAspectRatio();
								if (scale.x / scale.y != ratio)
									scale.x = scale.y * ratio;
							}
						}
						ImGui::PopStyleColor();
						if (isSelected)
							ImGui::SetItemDefaultFocus();
					}
					ImGui::EndCombo();
				}
				if (ImGui::IsItemClicked())
					AssetManager::ReloadAssetsList();

				// Tint color control
				ImGui::Dummy({ 0, 2 });
				ImGui::ColorEdit4("Color", glm::value_ptr(component.Color), ImGuiColorEditFlags_AlphaBar);
				ImGui::Dummy({ 0, 2 });

				if (sprite)
				{
					// Mirror flip
					ImGui::Text("Mirror Flip");
					ImGui::SameLine();
					ImGui::Checkbox("X##Flip", &sprite.m_MirrorFlip.x);
					ImGui::SameLine();
					ImGui::Checkbox("Y##Flip", &sprite.m_MirrorFlip.y);
					ImGui::Dummy({ 0, 1 });

					// Texture filter mode
					uint32_t filterMode = (uint32_t)sprite.GetTexture()->GetFilterMode();
					const char* filterModes[] = { "Nearest", "Linear" };

					if (ImGui::BeginCombo("Filter Mode", filterModes[filterMode]))
					{
						for (uint32_t i = 0; i < 2; i++)
						{
							const bool isSelected = (filterMode == i);
							if (ImGui::Selectable(filterModes[i], isSelected) && filterMode != i)
							{
								sprite.GetTexture()->SetFilterMode((TextureFilterMode)i);
								filterMode = i;
							}

							if (isSelected)
								ImGui::SetItemDefaultFocus();
						}
						ImGui::EndCombo();
					}

					// Tiling factor
					ImGui::DragFloat("Tiling Factor", &component.TilingFactor, 0.1f);
				}

				// Check if texture is spritesheet
				if (sprite && sprite.m_Spritesheet)
				{
					ImGui::Dummy({ 0, 5 });
					ImGui::Separator();
					ImGui::Text("Spritesheet");
					ImGui::Dummy({ 0, 3 });

					glm::ivec2 tilePos = (glm::ivec2)sprite.m_TilePos;
					if (ImGui::DragInt2("Tile Coords", glm::value_ptr(tilePos), 1, 0))
					{
						if (tilePos.x >= 0 && tilePos.y >= 0)
							sprite.SetTile(tilePos.x, tilePos.y);
					}

					glm::ivec2 tileSize = (glm::ivec2)sprite.m_TileSize;
					if (ImGui::DragInt2("Size", glm::value_ptr(tileSize), 0.2f, 1))
					{
						if (tileSize.x > 0 && tileSize.y > 0)
							sprite.SetTileSize((uint32_t)tileSize.x, (uint32_t)tileSize.y);
					}
				}
			});
		}

		// ******************************************************
		// ResizableSpriteComponent UI
		// ******************************************************
		if (selectedEntity.HasComponent<ResizableSpriteComponent>())
		{
			DrawComponentUI<ResizableSpriteComponent>("ResizableSprite", [&](auto& component)
				{
					auto& spritesheet = component.ResizableSprite.m_Spritesheet;
					auto& sprite = component.ResizableSprite;
					std::string filename = spritesheet 
						? GetFilepathRelative(s_TexturesPath, spritesheet->GetTexture()->GetPath())
						: "Select...";

					// Select spritesheet
					if (ImGui::BeginCombo("Spritesheet", filename.c_str()))
					{
						for (auto& kv : AssetManager::s_Instance->m_SpritesheetList)
						{
							bool isSelected = filename == kv.first;

							if (ImGui::Selectable(kv.first.c_str(), isSelected))
							{
								spritesheet = AssetManager::GetSpritesheet(kv.first);
								sprite.SetSpritesheet(spritesheet);
							}

							if (isSelected)
								ImGui::SetItemDefaultFocus();
						}
						ImGui::EndCombo();
					}
					if (ImGui::IsItemClicked())
						AssetManager::ReloadAssetsList();

					float tileScale = sprite.m_CellScale;
					if (ImGui::DragFloat("Tile Scale", &tileScale, 0.001f))
					{
						sprite.SetCellScale(tileScale);
					}
					ImGui::InputInt2("Pattern Offset", (int*)glm::value_ptr(sprite.m_PatternOffset));
					ImGui::InputInt2("Pattern Size", (int*)glm::value_ptr(sprite.m_PatternSize));
					ImGui::Dummy({ 0.0f, 3.0f });

					// Toggle sprite edges texture
					unsigned int bitset = (unsigned int)sprite.m_EdgesBitset;
					ImGui::Text("Toggle edge and corner texture:");
					ImGui::CheckboxFlags("##tb_top_left", &bitset, Edge_TopLeft);
					ImGui::SameLine();
					ImGui::CheckboxFlags("##tb_top", &bitset, Edge_Top);
					ImGui::SameLine();
					ImGui::CheckboxFlags("##tb_top_right", &bitset, Edge_TopRight);
							
					ImGui::CheckboxFlags("##tb_left", &bitset, Edge_Left);
					ImGui::SameLine(); 
					ImGui::Dummy({ 24.0f, 0.0f }); 
					ImGui::SameLine();
					ImGui::CheckboxFlags("##tb_right", &bitset, Edge_Right);

					ImGui::CheckboxFlags("##tb_bottom_left", &bitset, Edge_BottomLeft);
					ImGui::SameLine();
					ImGui::CheckboxFlags("##tb_bottom", &bitset, Edge_Bottom);
					ImGui::SameLine(); 
					ImGui::CheckboxFlags("##tb_bottom_right", &bitset, Edge_BottomRight);

					sprite.SetEdgesBitset((uint8_t)bitset);
					ImGui::Dummy({ 0, 3.0f });

					// Tint color control
					ImGui::ColorEdit4("Color", glm::value_ptr(component.Color), ImGuiColorEditFlags_AlphaBar);
					
					//if (ImGui::Button("Regenerate"))
					//	sprite.Generate(selectedEntity.GetTransform().Scale);
			});
		}

		// ******************************************************
		// Circle Renderer Component UI
		// ******************************************************
		if (selectedEntity.HasComponent<CircleRendererComponent>())
		{
			DrawComponentUI<CircleRendererComponent>("CircleRenderer", [&](auto& component)
				{
					ImGui::ColorEdit4("Color", glm::value_ptr(component.Color), ImGuiColorEditFlags_AlphaBar);
					ImGui::SliderFloat("Thickness", &component.Thickness, 0.0f, 1.0f);
					ImGui::SliderFloat("Fade", &component.Fade, 0.0f, 1.0f);
				});
		}


		// ******************************************************
		// Text Component UI
		// ******************************************************
		if (selectedEntity.HasComponent<TextComponent>())
		{
			DrawComponentUI<TextComponent>("Text", [&](auto& component)
				{
					static char textBuffer[2048] = "/0";
					strcpy_s(textBuffer, component.TextString.c_str());

					if (ImGui::InputTextMultiline("Text String", textBuffer, 2048))
						component.TextString = textBuffer;

					ImGui::ColorEdit4("Color", glm::value_ptr(component.Color));
					ImGui::DragFloat("Kerning", &component.Kerning, 0.025f);
					ImGui::DragFloat("Line Spacing", &component.LineSpacing, 0.025f);
					ImGui::Checkbox("Hidden", &component.Hidden);
				});
		}

		// ******************************************************
		// CameraComponent UI
		// ******************************************************
		if (selectedEntity.HasComponent<CameraComponent>())
		{
			DrawComponentUI<CameraComponent>("Camera", [&](auto& component)
				{
					bool isPrimary = scene->m_PrimaryCameraEntity == selectedEntity.m_Handle;
					if (ImGui::Checkbox("Set as primary", &isPrimary) && isPrimary)
						scene->SetPrimaryCameraEntity(selectedEntity);

					float zoom = component.Camera.GetZoomLevel();
					if (ImGui::DragFloat("Zoom Level", &zoom, 0.01f))
						component.Camera.SetZoomLevel(zoom);
					ImGui::DragFloat2("Offset", glm::value_ptr(component.PositionOffset), 0.01f);
				});
		}

		// ******************************************************
		// RigidbodyComponent UI
		// ******************************************************
		if (selectedEntity.HasComponent<RigidbodyComponent>())
		{
			DrawComponentUI<RigidbodyComponent>("Rigidbody", [scene, selectedEntity](auto& component)
			{
				b2Body* body = selectedEntity.GetRuntimeBody();

				std::string bodyType = "Static";
				if (component.Type == b2_dynamicBody)
					bodyType = "Dynamic";
				else if (component.Type == b2_kinematicBody)
					bodyType = "Kinematic";

				if (ImGui::BeginCombo("Body Type", bodyType.c_str()))
				{
					b2BodyType prev = component.Type;
					if (ImGui::Selectable("Static"))
						component.Type = b2_staticBody;
					else if (ImGui::Selectable("Dynamic"))
						component.Type = b2_dynamicBody;
					else if (ImGui::Selectable("Kinematic"))
						component.Type = b2_kinematicBody;

					if (scene->IsSimulated() && body && prev != component.Type)
						body->SetType(component.Type);

					ImGui::EndCombo();
				}

				ImGui::Dummy({ 0.0f, 3.0f });

				if (scene->IsSimulated() && body)
				{
					float linearDamping = body->GetLinearDamping();
					if (ImGui::DragFloat("Linear Damping", &linearDamping, 0.01f))
					{
						body->SetLinearDamping(linearDamping);
						component.LinearDamping = linearDamping;
					}
					
					float angularDamping = body->GetAngularDamping();
					if (ImGui::DragFloat("Angular Damping", &angularDamping, 0.01f))
					{
						body->SetAngularDamping(angularDamping);
						component.AngularDamping = angularDamping;
					}
					
					float gravityScale = body->GetGravityScale();
					if (ImGui::DragFloat("Gravity Scale", &gravityScale, 0.01f))
					{
						body->SetGravityScale(gravityScale);
						component.GravityScale = gravityScale;
					}

					ImGui::Dummy({ 0.0f, 3.0f });

					bool fixedRotation = body->IsFixedRotation();
					if (ImGui::Checkbox("Fixed rotation", &fixedRotation))
					{
						body->SetFixedRotation(fixedRotation);
						component.FixedRotation = fixedRotation;
					}

					bool isBullet = body->IsBullet();
					if (ImGui::Checkbox("Is Bullet", &isBullet))
					{
						body->SetBullet(isBullet);
						component.IsBullet = isBullet;
					}
				}
				else
				{
					ImGui::DragFloat("Linear Damping", &component.LinearDamping, 0.01f);
					ImGui::DragFloat("Angular Damping", &component.AngularDamping, 0.01f);
					ImGui::DragFloat("Gravity Scale", &component.GravityScale, 0.01f);
					
					ImGui::Dummy({ 0.0f, 3.0f });
					ImGui::Checkbox("Fixed rotation", &component.FixedRotation);
					ImGui::Checkbox("Is Bullet", &component.IsBullet);
				}
				
				ImGui::Checkbox("Attach to parent", &component.AttachToParent);
			});
		}

		// ******************************************************
		// BoxColliderComponent UI
		// ******************************************************
		if (selectedEntity.HasComponent<BoxColliderComponent>())
		{
			DrawComponentUI<BoxColliderComponent>("BoxCollider", [](auto& component)
			{
				ImGui::Checkbox("Attach to parent", &component.AttachToParent);
				ImGui::DragFloat2("Size", glm::value_ptr(component.Size), 0.01f);
				ImGui::DragFloat2("Offset", glm::value_ptr(component.Offset), 0.01f);
				ImGui::DragFloat("Friction", &component.Material.Friction, 0.01f);
				ImGui::DragFloat("Restitution", &component.Material.Restitution, 0.01f);
				ImGui::DragFloat("RestitutionThreshold", &component.Material.RestitutionThreshold, 0.01f);
				ImGui::DragFloat("Density", &component.Material.Density, 0.01f);
				ImGui::Checkbox("IsSensor", &component.IsSensor);
			});
		}

		// ******************************************************
		// CircleColliderComponent UI
		// ******************************************************
		if (selectedEntity.HasComponent<CircleColliderComponent>())
		{
			DrawComponentUI<CircleColliderComponent>("CircleCollider", [](auto& component)
			{
				ImGui::Checkbox("Attach to parent", &component.AttachToParent);
				ImGui::DragFloat2("Offset", glm::value_ptr(component.Offset), 0.01f);
				ImGui::DragFloat("Radius", &component.Radius, 0.001f);
				ImGui::DragFloat("Friction", &component.Material.Friction, 0.01f);
				ImGui::DragFloat("Restitution", &component.Material.Restitution, 0.01f);
				ImGui::DragFloat("RestitutionThreshold", &component.Material.RestitutionThreshold, 0.01f);
				ImGui::DragFloat("Density", &component.Material.Density, 0.01f);
				ImGui::Checkbox("IsSensor", &component.IsSensor);
			});
		}

		// ******************************************************
		// NetworkComponent UI
		// ******************************************************
		if (selectedEntity.HasComponent<NetworkComponent>())
		{
			DrawComponentUI<NetworkComponent>("Network", [](NetworkComponent& component)
			{
				ImGui::Checkbox("Simulate on client", &component.SimulateOnClient);
				ImGui::PushItemWidth(200.0f);
				auto& netTransform = component.NetTransform;
				auto& selectedMethod = netTransform.Method;
				if (ImGui::BeginCombo("Sync Method", NetSyncMethodToString(selectedMethod).c_str()))
				{
					for (uint8_t i = 0; i < 4; i++)
					{
						auto current = (NetSyncMethod)i;

						if (ImGui::Selectable(NetSyncMethodToString(current).c_str(), selectedMethod == current))
							selectedMethod = current;
					}
					ImGui::EndCombo();
				}
				ImGui::PopItemWidth();

				ImGui::Dummy({ 0, 5 });
				ImGui::PushItemWidth(125.0f);
				float cullDistance = netTransform.CullDistance;
				if (ImGui::DragFloat("Cull Distance", &cullDistance, 0.1f))
					netTransform.CullDistance = glm::clamp(cullDistance, 0.0f, 1000.0f);
				ImGui::DragFloat("Teleport Threshold", &netTransform.TeleportThreshold, 0.001f);
				ImGui::DragFloat("Reconcile Threshold", &netTransform.ReconcileThreshold, 0.001f);
				ImGui::DragFloat("Reconcile Max Time", &netTransform.ReconcileMaxTime, 0.001f);
				ImGui::PopItemWidth();

				ImGui::Dummy({ 0, 5 });
				if (ImGui::TreeNode("Debug"))
				{
					ImGui::Dummy({ 0, 2 });
					ImGui::Text("Replication Timer: %.3f", netTransform.ReplicationTimer);
					ImGui::Text("Interpolation Timer: %.3f", netTransform.InterpolationTimer);
					ImGui::Text("Reconcile Timer: %.3f", netTransform.ReconcileTimer);
					ImGui::Text("Reconcile State: pos=%d, sca=%d, rot=%d",
						netTransform.IsReconciling(NetTransform::Components::Position),
						netTransform.IsReconciling(NetTransform::Components::Scale),
						netTransform.IsReconciling(NetTransform::Components::Rotation));
					ImGui::Text("Current Sequence Number: %d", netTransform.CurrentSequenceNumber);
					ImGui::Text("Server Sequence Number: %d", netTransform.ServerSequenceNumber);
					ImGui::Text("Delta Buffer Size: %d", netTransform.DeltaBuffer.size());

					ImGui::Dummy({ 0, 5 });
					float lastPos[] = { netTransform.LastAuthoritativeTransform.Position.x, netTransform.LastAuthoritativeTransform.Position.y };
					float prevPos[] = { netTransform.PrevAuthoritativeTransform.Position.x, netTransform.PrevAuthoritativeTransform.Position.y };
					float predPos[] = { netTransform.PredictedTransform.Position.x, netTransform.PredictedTransform.Position.y };
					ImGui::Dummy({ 0, 2 });
					ImGui::Text("Position");
					ImGui::DragFloat2("Last", lastPos);
					ImGui::DragFloat2("Previous", prevPos);
					ImGui::DragFloat2("Predicted", predPos);

					float lastScale[] = { netTransform.LastAuthoritativeTransform.Scale.x, netTransform.LastAuthoritativeTransform.Scale.y };
					float prevScale[] = { netTransform.PrevAuthoritativeTransform.Scale.x, netTransform.PrevAuthoritativeTransform.Scale.y };
					float predScale[] = { netTransform.PredictedTransform.Scale.x, netTransform.PredictedTransform.Scale.x };
					ImGui::Dummy({ 0, 2 });
					ImGui::Text("Scale");
					ImGui::DragFloat2("Last", lastScale);
					ImGui::DragFloat2("Previous", prevScale);
					ImGui::DragFloat2("Predicted", predScale);

					float prevRot = netTransform.PrevAuthoritativeTransform.Rotation;
					float lastRot = netTransform.LastAuthoritativeTransform.Rotation;
					float predRot = netTransform.PredictedTransform.Rotation;
					ImGui::Dummy({ 0, 5 });
					ImGui::Text("Rotation");
					ImGui::DragFloat("Last", &lastRot);
					ImGui::DragFloat("Previous", &prevRot);
					ImGui::DragFloat("Predicted", &predRot);

					ImGui::TreePop();
				}
			});
		}

		ImGui::End();
	}

	void InspectorPanel::DrawSceneProporties()
	{
		Scene* scene = GetActiveScene();
		Entity selectedEntity = GetSelectedEntity();

		ImGui::Text("Scene Proporties");
		ImGui::Dummy({ 0.0f, 2.0f });
		ImGui::Separator();
		ImGui::Dummy({ 0.0f, 5.0f });

		if (ImGui::TreeNodeEx("General", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::Dummy({ 0, 2 });
			const std::string& selectedGameMode = scene->m_GameModeClassName;
			ImGui::PushItemWidth(210.0f);
			if (ImGui::BeginCombo("GameMode Class", selectedGameMode.c_str()))
			{
				for (auto& [className, instanciateFunc] : ScriptFactory::Get().m_GameModeRegistry)
				{
					bool selected = selectedGameMode == className;
					if (ImGui::Selectable(className.c_str(), selected))
					{
						scene->SetGameModeByClassName(className);
					}
				}
				ImGui::EndCombo();
			}

			ImGui::Dummy({ 0.0f, 5.0f });
			if (ImGui::ColorEdit4("Clear Color", glm::value_ptr(scene->m_ClearColor)))
				Renderer::SetClearColor(scene->m_ClearColor);
			ImGui::PopItemWidth();

			ImGui::TreePop();
		}
		ImGui::Dummy({ 0, 5 });

		if (ImGui::TreeNodeEx("Physics", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::Dummy({ 0, 2 });

			bool enablePhysics = scene->m_EnablePhysics;
			if (ImGui::Checkbox("Enable Physics", &enablePhysics) && scene->m_State == SceneState::Stop)
				scene->m_EnablePhysics = enablePhysics;

			if (scene->m_EnablePhysics)
			{
				ImGui::Dummy({ 0,5 });
				ImGui::PushItemWidth(100.0f);
				
				if (ImGui::DragFloat("Physics Tickrate", &scene->m_PhysicsTickrate, 1.0f, 32.0f, 256.0f, "%.0f"))
					scene->SetPhysicsTickrate(scene->m_PhysicsTickrate);
				
				ImGui::DragFloat("World Gravity", &scene->m_PhysicsWorld->m_Gravity, 0.1f);

				int* vi = &scene->m_PhysicsWorld->m_PhysicsVelocityIterations;
				int* pi = &scene->m_PhysicsWorld->m_PhysicsPositionIterations;
				if (ImGui::DragInt("Velocity Iterations", vi))
					*vi = glm::max(*vi, 1);
				if (ImGui::DragInt("Position Iterations", pi))
					*pi = glm::max(*pi, 1);

				ImGui::PopItemWidth();
			}

			ImGui::TreePop();
		}
		ImGui::Dummy({ 0, 5 });

		if (ImGui::TreeNodeEx("Network", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::Dummy({ 0, 2 });
			ImGui::Checkbox("Enable Networking", &scene->m_EnableNetworking);

			ImGui::TreePop();
		}
	}

	template<typename T>
	void InspectorPanel::DrawComponentUI(const std::string& name, const std::function<void(T&)>& drawContentFunction)
	{
		Entity selectedEntity = GetSelectedEntity();
		T& component = selectedEntity.GetComponent<T>();

		ImGuiTreeNodeFlags treeNodeFlags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanAvailWidth
			| ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_FramePadding;
		
		bool opened = ImGui::TreeNodeEx((void*)typeid(T).hash_code(), treeNodeFlags, name.c_str());

		bool removeComponent = false;
		if (!std::is_same<T, TagComponent>::value && !std::is_same<T, TransformComponent>::value)
		{
			removeComponent = DrawTreeNodeRemoveButton(name);
		}

		NetworkComponent* networkComponent = nullptr;
		if (selectedEntity.HasComponent<NetworkComponent>())
			networkComponent = &selectedEntity.GetComponent<NetworkComponent>();

		if (opened)
		{
			ImGui::Dummy({ 0.0f, 3.0f });
			drawContentFunction(component);
			ImGui::TreePop();
		}

		if (removeComponent)
			selectedEntity.RemoveComponent<T>();

		ImGui::Dummy({ 0.0f, 3.0f });
	}

}

#endif // PT_EDITOR
