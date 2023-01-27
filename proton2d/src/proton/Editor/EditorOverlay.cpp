#include "pch.h"
#include "proton/Editor/EditorOverlay.h"
#include "proton/Core/Application.h"
#include "proton/Core/Window.h"
#include "proton/Scene/Components.h"
#include "proton/Assets/SceneSerializer.h"
#include "proton/Core/Utils.h"
#include "proton/Events/MouseEvents.h"
#include "proton/Events/KeyEvents.h"
#include "proton/Scene/SceneManager.h"
#include "proton/Graphics/Renderer.h"
#include "proton/Core/Utils.h"
#include "proton/Scene/PrefabManager.h"

#define IMGUI_IMPL_OPENGL_LOADER_GLAD
#include <backends/imgui_impl_opengl3.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.cpp>
#include <backends/imgui_impl_glfw.cpp>

#include <GLFW/glfw3.h>
#include <imgui.h>
#include <fstream>

namespace proton {

	EditorOverlay* EditorOverlay::s_Instance = nullptr;

	EditorOverlay::EditorOverlay()
	{
		EditorOverlay::s_Instance = this;
	}

	void EditorOverlay::OnCreate()
	{
		// ImGui setup
		ImGui::CreateContext();
		ImGui::StyleColorsDark();

		auto& io = ImGui::GetIO();
		io.ConfigFlags = ImGuiConfigFlags_DockingEnable | ImGuiConfigFlags_NavEnableKeyboard;// | ImGuiConfigFlags_ViewportsEnable;
		io.Fonts->AddFontFromFileTTF("assets/Roboto.ttf", 18);

		ImGuiStyle& style = ImGui::GetStyle();
		style.FrameRounding = 7.0f;

		auto window = (GLFWwindow*)Application::Get().GetWindow().GetNativeWindow();
		ImGui_ImplGlfw_InitForOpenGL(window, true);
		ImGui_ImplOpenGL3_Init("#version 410");
	}

	void EditorOverlay::OnDestroy()
	{
		ImGui_ImplOpenGL3_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();
	}

	void EditorOverlay::OnUpdate(float ts)
	{
		m_Camera.OnUpdate(ts);

		if (m_Inspector.m_SelectedEntity.IsValid())
		{
			if (m_MovingSelection)
			{
				glm::vec2 targetPos = m_ActiveScene->GetMouseWorldPosition() + m_SelectionMouseOffset;
				auto& transform = m_Inspector.m_SelectedEntity.GetComponent<TransformComponent>();
				transform.Position = { targetPos.x, targetPos.y, transform.Position.z };
				ImGui::SetMouseCursor(7);
			}
		}
		else
			m_Inspector.SetSelectionContext(Entity{});

		if (m_MovingCamera)
		{
			glm::vec2 mousePos = m_ActiveScene->GetMouseWorldPosition();
			glm::vec2 offset = m_CameraDragOffset - mousePos;
			m_Camera.m_Position.x += offset.x;
			m_Camera.m_Position.y += offset.y;
			ImGui::SetMouseCursor(2);
		}

		if (m_SavedSceneTextTimer == 1.5f)
			m_SavedSceneTextTimer = 1.499f; // skip first timer update
		else
			m_SavedSceneTextTimer = glm::max(m_SavedSceneTextTimer - ts, 0.0f);
	}

	void EditorOverlay::OnImGuiRender()
	{
		//ImGui::ShowDemoWindow(); // Demo window for reference

		//************************************
		//   Entity Hierarchy 
		//************************************
		ImGui::Begin("Hierarchy");
		ImGui::Dummy({0.0f, 1.0f});

		ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_OpenOnArrow;

		if (!m_Inspector.m_SelectedEntity)
			flags |= ImGuiTreeNodeFlags_Selected;

		bool opened = ImGui::TreeNodeEx(m_ActiveScene->m_SceneName.c_str(), flags);
			
		if (ImGui::IsItemClicked())
			m_Inspector.SetSelectionContext(Entity{});

		if (ImGui::IsItemClicked(1))
			ImGui::OpenPopup("new_entity_root");
		if (ImGui::BeginPopup("new_entity_root"))
		{
			if (ImGui::MenuItem("Create new entity"))
				m_ActiveScene->CreateEntity();
			ImGui::EndPopup();
		}

		if (ImGui::BeginDragDropTarget())
		{
			if (ImGui::AcceptDragDropPayload("Entity"))
				m_DraggedEntity.PopHierarchy();
			ImGui::EndDragDropTarget();
		}

		if (opened)
		{
			m_ActiveScene->m_Registry.each([&](auto id)
			{
				Entity entity{ m_ActiveScene, id};
				auto& relationship = entity.GetComponent<RelationshipComponent>();

				if (relationship.Parent == entt::null)
					DrawEntityTreeNode(entity);
			});
				
			ImGui::TreePop();
		}

		//************************************
		//   Draw other panels
		//************************************
		m_DebugInfo.m_EntitiesCount = m_ActiveScene->GetEntitiesCount();
		m_DebugInfo.m_ScriptedEntitiesCount = m_ActiveScene->GetScriptedEntitiesCount();

		m_Inspector.OnImGuiRender();
		m_DebugInfo.OnImGuiRender();

		DrawScenePanel();
		DrawCollidersAndSelectionOutline();
		DrawPrefabPanel();
		ImGui::End();
	}

	void EditorOverlay::OnEvent(Event& event)
	{
		m_Camera.OnEvent(event);

		EventDispatcher dispatcher(event);
		dispatcher.Dispatch<MouseButtonPressedEvent>([&](MouseButtonPressedEvent& e)
		{
			if (ImGui::GetIO().WantCaptureMouse)
				return false;

			if (e.GetMouseButton() == Mouse::Button1 && !m_MovingCamera)
			{
				// Move editor camera on mouse pressed event (Button 1)
				m_CameraDragOffset = m_ActiveScene->GetMouseWorldPosition();
				m_MovingCamera = true;
			}
			else if (e.GetMouseButton() == Mouse::Button0)
			{
				// Select entity on mouse pressed event (Button 0)
				glm::vec2 mousePos = m_ActiveScene->GetMouseWorldPosition();
				Entity& selectedEntity = m_Inspector.m_SelectedEntity;

				std::vector<Entity> entities = m_ActiveScene->GetEntitiesOnMousePosition();
				Entity target; float transformMaxZ = 0.0f;

				for (auto& entity : entities)
				{
					auto& transform = entity.GetComponent<TransformComponent>();
					if (!target || transform.Position.z > transformMaxZ)
					{
						target = entity;
						transformMaxZ = transform.Position.z;
					}
				}

				if (selectedEntity && m_ActiveScene->IsMouseHoveringEntity(selectedEntity))
				{
					auto& transform = selectedEntity.GetComponent<TransformComponent>();
					auto& targetTransform = target.GetComponent<TransformComponent>();
					if (transform.Scale.x < targetTransform.Scale.x
						&& transform.Scale.y < targetTransform.Scale.y)
					{
						// Discard selection
						target = selectedEntity;
					}	
				}

				if (target && target == selectedEntity)
				{
					m_MovingSelection = true;
					auto& transform = selectedEntity.GetComponent<TransformComponent>();
					m_SelectionMouseOffset = glm::vec2{ transform.Position.x, transform.Position.y } - mousePos;
				}

				SetInspectorContext(target);
			}
			return true;
		});

		dispatcher.Dispatch<KeyPressedEvent>([&](KeyPressedEvent& e)
		{
			if (ImGui::GetIO().WantTextInput)
				return false;

			if (e.GetKeyCode() == Key::F1)
				m_ShowSelectionOutline = !m_ShowSelectionOutline;
			if (e.GetKeyCode() == Key::F2)
				m_ShowSelectionCollider = !m_ShowSelectionCollider;
			if (e.GetKeyCode() == Key::F3)
				m_ShowAllColliders = !m_ShowAllColliders;

			if (e.GetKeyCode() == Key::Delete)
			{
				Entity selectedEntity = GetInspectorContext();
				if (selectedEntity)
				{
					selectedEntity.Destroy();
					SetInspectorContext({});
				}
			}
			return true;
		});

		dispatcher.Dispatch<MouseButtonReleasedEvent>([&](MouseButtonReleasedEvent& e)
		{
			if (e.GetMouseButton() == Mouse::Button0)
				m_MovingSelection = false;
			if (e.GetMouseButton() == Mouse::Button1)
				m_MovingCamera = false;

			ImGui::SetMouseCursor(0);
			return true;
		});

	}

	void EditorOverlay::DrawEntityTreeNode(Entity entity)
	{
		auto& relationship = entity.GetComponent<RelationshipComponent>();
		
		ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_OpenOnArrow;
		
		if (m_Inspector.m_SelectedEntity == entity)
			flags |= ImGuiTreeNodeFlags_Selected;

		if (relationship.First == entt::null)
			flags |= ImGuiTreeNodeFlags_Leaf;

		bool opened = ImGui::TreeNodeEx((void*)(uint64_t)(uint32_t)entity, flags, entity.GetComponent<TagComponent>().Tag.c_str());

		if (ImGui::IsItemHovered() && ImGui::IsMouseDragging(0))
		{
			m_DraggedEntity = entity;
			ImGui::BeginDragDropSource();
			ImGui::SetDragDropPayload("Entity", (void*)&m_DraggedEntity, sizeof(Entity));
			ImGui::EndDragDropSource();
		}

		if (ImGui::BeginDragDropTarget())
		{
			if (!m_DraggedEntity.IsParentOf(entity) && ImGui::AcceptDragDropPayload("Entity"))
			{
				m_DraggedEntity.PopHierarchy();
				entity.AddChildEntity(m_DraggedEntity);
			}
			ImGui::EndDragDropTarget();
		}

		if (ImGui::IsItemClicked())
			m_Inspector.SetSelectionContext(entity);

		if (ImGui::IsItemClicked(1))
			ImGui::OpenPopup("new_entity_child");
		if (ImGui::BeginPopup("new_entity_child"))
		{
			if (ImGui::MenuItem("Create new entity"))
				entity.AddChildEntity(m_ActiveScene->CreateEntity());
			ImGui::EndPopup();
		}

		if (opened)
		{
			if (relationship.ChildrenCount)
			{
				auto current = relationship.First;
				for (uint32_t i = 0; i < relationship.ChildrenCount; i++)
				{
					Entity e{ m_ActiveScene, current };
					DrawEntityTreeNode(e);
					current = e.GetComponent<RelationshipComponent>().Next;
				}
			}
			ImGui::TreePop();
		}
	}

	static std::string GetSceneFilename(const std::string& filepath) 
	{
		std::size_t pos = filepath.find("scenes");
		if (pos != std::string::npos) {
			std::string filename = filepath.substr(pos + 7);
			std::size_t posExt = filepath.find(".scene");
			if (posExt != std::string::npos) 
				return filename.substr(0, filename.size() - 6);
			return filename;
		}
		return std::string();
	}

	void EditorOverlay::DrawScenePanel()
	{
		ImGui::Begin("Scene");
		ImGui::Dummy({ 0, 1.0f });

		bool editMode = m_ActiveScene->m_SceneState == SceneState::Edit;
		ImGui::SetCursorPosX((ImGui::GetWindowWidth() - (editMode ? 75 : 160)) / 2.0f);

		bool stopSimulaton = false;
		// Play / stop buttons
		if (!editMode)
		{
			if (ImGui::Button("Stop", { 75, 30 }))
				stopSimulaton = true;

			ImGui::SameLine();
			if (m_ActiveScene->GetSceneState() == SceneState::Paused)
			{
				if (ImGui::Button("Resume", { 75, 30 }))
					m_ActiveScene->Pause(false);
			}
			else
			{
				if (ImGui::Button("Pause", { 75, 30 }))
					m_ActiveScene->Pause(true);
			}
		}
		else 
		{
			if (ImGui::Button("Play", { 75, 30 }))
				m_ActiveScene->BeginPlay();
		}

		if (stopSimulaton)
		{
			std::string filepath = m_ActiveScene->GetFilepath();
			if (filepath.size())
			{
				SceneManager::LoadFromCache(filepath);
				SceneManager::SetActiveScene(filepath);
			}
		}

		ImGui::Dummy({ 0, 5 }); ImGui::Separator(); ImGui::Dummy({ 0, 1 });

		std::string sceneText = "Scene: " + m_ActiveScene->m_SceneFilepath;
		if (m_SavedSceneTextTimer > 0.0f)
			sceneText = "[Saved] " + sceneText;
		ImGui::SetCursorPosX((ImGui::GetWindowWidth() - ImGui::CalcTextSize(sceneText.c_str()).x) / 2);
		ImGui::Text(sceneText.c_str());
		ImGui::Dummy({ 0, 3 });

		// Center buttons
		ImGui::SetCursorPosX(ImGui::GetWindowWidth() / 2 - 105);

		bool saveAs = false;
		bool createNewScene = false;

		// New scene
		if (ImGui::Button("New scene", { 100, 25 }))
		{
			ImGui::OpenPopup("new_scene_confirm");
		}
		if (ImGui::BeginPopup("new_scene_confirm"))
		{
			ImGui::Text("Save current scene?");
			if (ImGui::Button("Yes"))
			{
				createNewScene = true;
				if (m_ActiveScene->m_SceneFilepath != "<Unsaved scene>")
					SceneManager::SaveActiveScene();
				else 
					saveAs = true;
				ImGui::CloseCurrentPopup();
			}
			ImGui::SameLine();
			if (ImGui::Button("No"))
			{
				createNewScene = true;
				ImGui::CloseCurrentPopup();
			}
			ImGui::SameLine();
			if(ImGui::Button("Cancel"))
				ImGui::CloseCurrentPopup();

			ImGui::EndPopup();
		}

		// Open scene
		ImGui::SameLine();
		if (ImGui::Button("Open scene", { 100, 25 }))
		{
			std::string sceneFile = GetSceneFilename(FileDialogs::OpenFile("scene"));
			if (sceneFile.size())
			{
				SceneManager::Load(sceneFile);
				SceneManager::SetActiveScene(sceneFile);
			}
		}

		ImGui::Dummy({ 0, 5.0f });
		ImGui::SetCursorPosX(ImGui::GetWindowWidth() / 2 - 105);

		// Save
		if (ImGui::Button("Save", { 100, 25 }))
		{
			if (m_ActiveScene->m_SceneFilepath != "<Unsaved scene>")
			{
				SceneManager::SaveActiveScene();
				//m_SavedSceneTextTimer = 1.5f;
			}
			else
				saveAs = true;
		}
		
		// Save as
		ImGui::SameLine();
		if (ImGui::Button("Save as", { 100, 25 }))
			saveAs = true;

		// Save as and Create new scene logic
		if (saveAs)
		{
			std::string sceneFile = GetSceneFilename(FileDialogs::SaveFile(".scene"));
			if (sceneFile.size())
			{
				SceneManager::SaveActiveSceneAs(sceneFile);
				if (m_ActiveScene->m_SceneFilepath == "<Unsaved scene>")
				{
					if (!createNewScene)
						SceneManager::Unload("<Unsaved scene>");
				}
				SceneManager::Load(sceneFile);
				SceneManager::SetActiveScene(sceneFile);
				m_SavedSceneTextTimer = 1.5f;
			}
		}
		if (createNewScene)
		{
			Scene* scene = SceneManager::CreateEmptyScene("<Unsaved scene>");
			SceneManager::s_Instance->m_ActiveScene = scene;
			SetSceneContext(scene);
		}

		// ****************************************************

		ImGui::Dummy({ 0, 5 });
		ImGui::Separator();
		ImGui::Dummy({ 0, 2 });
		if (ImGui::TreeNodeEx("Scenes loaded in memory", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::Dummy({ 0, 2 });
			for (auto& [sceneName, scenePtr] : SceneManager::s_Instance->m_Scenes)
			{
				ImGui::Text(sceneName.c_str());
				bool isActive = m_ActiveScene == scenePtr;

				ImGui::SameLine(ImGui::GetWindowWidth() - (isActive ? 140 : 160));
				if (!isActive && ImGui::Button(("Set active##" + sceneName).c_str()))
					SceneManager::SetActiveScene(sceneName);

				if (isActive)
				{
					ImGui::PushStyleColor(ImGuiCol_Text, { 0.0f, 1.0f, 0.2f, 1.0f });
					ImGui::Text("Currenty active");
					ImGui::PopStyleColor();
					ImGui::Dummy({ 0, 2 });
				}
				else
				{
					ImGui::SameLine();
					if (ImGui::Button(("Unload##" + sceneName).c_str()))
					{
						SceneManager::Unload(sceneName);
						break;
					}
				}
			}
			ImGui::TreePop();
		}
		ImGui::End();
	}

	void EditorOverlay::DrawPrefabPanel()
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
			if (ImGui::Button(("Spawn##" + tag).c_str(), {60, 25}))
			{
				Entity entity = PrefabManager::SpawnPrefab(m_ActiveScene, tag);
				auto& transform = entity.GetComponent<TransformComponent>();
				glm::vec2 cameraPos = m_ActiveScene->GetPrimaryCameraPosition();
				transform.Position.x = cameraPos.x;
				transform.Position.y = cameraPos.y;
				if (m_Inspector.m_SelectedEntity)
					m_Inspector.m_SelectedEntity.AddChildEntity(entity);
			}
			ImGui::SameLine();
			if (ImGui::Button(("X##" + tag).c_str(), {25, 25}))
				deletePrefabTag = tag;
		}
		if (deletePrefabTag.size())
			PrefabManager::DeletePrefab(deletePrefabTag);
		ImGui::Separator();
		ImGui::End();
	}

	void EditorOverlay::DrawCollidersAndSelectionOutline()
	{
		Renderer::BeginScene(*m_ActiveScene->GetPrimaryCamera(), m_ActiveScene->GetPrimaryCameraPosition());
		// Draw box colliders
		auto view = m_ActiveScene->m_Registry.view<TransformComponent, BoxColliderComponent>();
		for (auto entity : view)
		{
			auto [transform, bc] = view.get<TransformComponent, BoxColliderComponent>(entity);

			// Check if current entity is selected entity and show selection collider is enabled
			bool drawSelected = m_ShowSelectionCollider && GetInspectorContext().m_Handle == entity;

			if (m_ShowAllColliders || drawSelected)
			{
				float zPos = (m_ShowAllColliders && drawSelected) ? 0.205f : 0.2f;
				glm::vec4 color = (m_ShowAllColliders && drawSelected) 
					? glm::vec4{ 0.9f, 0.3f, 0.3f, 0.5f } : glm::vec4{ 0.9f, 0.6f, 0.3f, 0.5f };
				glm::vec3 position = { transform.Position.x + bc.Offset.x, transform.Position.y + bc.Offset.y, zPos };
				glm::vec3 scale = { bc.Size.x * transform.Scale.x, bc.Size.y * transform.Scale.y, 1.0f };
				glm::mat4 transformMatrix = Math::GetTransform(position, scale, transform.Rotation);

				Renderer::DrawQuad(transformMatrix, color);
			}
		}

		// Draw selected entity outline
		Entity selectedEntity = GetInspectorContext();

		if (selectedEntity.IsValid() && m_ShowSelectionOutline)
		{
			auto& transform = selectedEntity.GetComponent<TransformComponent>();
			float padding = glm::sqrt(m_ActiveScene->GetPrimaryCamera()->GetZoomLevel()) * 0.05f;
			glm::vec3 position = { transform.Position.x, transform.Position.y, 0.21f };
			glm::vec3 scale = { transform.Scale.x + padding, transform.Scale.y + padding, 1.0f };
			glm::mat4 transformMatrix = Math::GetTransform(position, scale, transform.Rotation);

			glm::vec4 color = m_ShowSelectionOutline && m_MovingSelection
				? glm::vec4{ 0.8f, 0.8f, 0.2f, 1.0f } : glm::vec4{ 1.0f };

			if (selectedEntity.HasComponent<SpriteComponent>())
			{
				auto& sprite = selectedEntity.GetComponent<SpriteComponent>();
				if (sprite.Sprite)
					scale.x *= (float)sprite.Sprite.m_PixelSize.x / (float)sprite.Sprite.m_PixelSize.y;
			}
			Renderer::SetLineWidth(glm::min(50.0f * padding, 1.0f));
			Renderer::DrawRect(transformMatrix, color);
		}
		Renderer::EndScene();
	}

	void EditorOverlay::ResetCameraPosition()
	{
		m_Camera.m_Position = { 0.0f, 0.0f, 0.0f };
	}

	void EditorOverlay::SetSceneContext(Scene* context)
	{
#if PROTON_EDITOR
		s_Instance->m_ActiveScene = context;
		s_Instance->m_Inspector.m_ActiveScene = context;
#endif
	}

	void EditorOverlay::SetInspectorContext(Entity entity)
	{
#if PROTON_EDITOR
		s_Instance->m_Inspector.SetSelectionContext(entity);
#endif
	}

	Entity EditorOverlay::GetInspectorContext()
	{
#if PROTON_EDITOR
		return s_Instance->m_Inspector.m_SelectedEntity;
#else
		return {};
#endif
	}

	void EditorOverlay::BeginImGuiRender()
	{
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();
	}

	void EditorOverlay::EndImGuiRender()
	{
		auto& io = ImGui::GetIO();
		auto& window = Application::Get().GetWindow();
		io.DisplaySize = { (float)window.GetWidth(), (float)window.GetHeight() };

		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
	}

}
