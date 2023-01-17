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

#include "proton/Platform/Windows/FileDialogs.h"

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
		io.Fonts->AddFontFromFileTTF(PROTON_ENGINE_ASSETS_DIR "/Roboto.ttf", 18);

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
		if (!m_ActiveScene)
			return;

		m_Camera.OnUpdate(ts);

		if (!m_Inspector.m_SelectedEntity.IsValid())
			m_Inspector.SetSelectionContext(Entity{});

		if (m_MovingSelection)
		{
			glm::vec2 targetPos = m_ActiveScene->GetMouseWorldPosition() + m_SelectionMouseOffset;
			auto& transform = m_Inspector.m_SelectedEntity.GetComponent<TransformComponent>();
			transform.Position = { targetPos.x, targetPos.y, transform.Position.z };
			ImGui::SetMouseCursor(2);
		}
		if (m_MovingCamera)
		{
			glm::vec2 mousePos = m_ActiveScene->GetMouseWorldPosition();
			glm::vec2 offset = m_CameraMoveClickPosition - mousePos;
			m_Camera.m_Position.x += offset.x;
			m_Camera.m_Position.y += offset.y;
			ImGui::SetMouseCursor(7);
		}
	}

	void EditorOverlay::OnImGuiRender()
	{
		ImGui::ShowDemoWindow(); // Demo window for reference

		//************************************
		//   Entity Hierarchy 
		//************************************
		ImGui::Begin("Hierarchy");
		if (m_ActiveScene)
		{
			ImGui::Dummy({0.0f, 1.0f});
			if (ImGui::Button("Create new entity", {340.0f, 25.0f}))
			{
				Entity entity = m_ActiveScene->CreateEntity();
				if (m_Inspector.m_SelectedEntity)
					m_Inspector.m_SelectedEntity.AddChildEntity(entity);
				m_Inspector.SetSelectionContext(entity);
			}
			ImGui::Dummy({0.0f, 1.0f});

			ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_OpenOnArrow;

			if (!m_Inspector.m_SelectedEntity)
				flags |= ImGuiTreeNodeFlags_Selected;

			bool opened = ImGui::TreeNodeEx(m_ActiveScene->m_SceneName.c_str(), flags);
			
			if (ImGui::IsItemClicked())
				m_Inspector.SetSelectionContext(Entity{});

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

			DrawSceneSerializationPanel();
			DrawCollidersAndSelectionOutline();
			DrawPrefabPanel();
		}
		ImGui::End();
	}

	void EditorOverlay::OnEvent(Event& event)
	{
		m_Camera.OnEvent(event);

		EventDispatcher dispatcher(event);
		dispatcher.Dispatch<MouseButtonPressedEvent>([&](MouseButtonPressedEvent& e)
		{
			if (!m_ActiveScene || ImGui::GetIO().WantCaptureMouse)
				return false;

			if (e.GetMouseButton() == Mouse::Button1 && !m_MovingCamera)
			{
				// Move editor camera on mouse pressed event (Button 1)
				m_CameraMoveClickPosition = m_ActiveScene->GetMouseWorldPosition();
				m_MovingCamera = true;
			}
			else if (e.GetMouseButton() == Mouse::Button0)
			{
				// Select entity on mouse pressed event (Button 0)
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
				if (target && target == m_Inspector.m_SelectedEntity)
				{
					m_MovingSelection = true;
					glm::vec2 mousePos = m_ActiveScene->GetMouseWorldPosition();
					auto& transform = m_Inspector.m_SelectedEntity.GetComponent<TransformComponent>();
					m_SelectionMouseOffset = glm::vec2{ transform.Position.x, transform.Position.y } - mousePos;
				}

				
				SetInspectorContext(target);
			}
			return true;
		});

		dispatcher.Dispatch<KeyPressedEvent>([&](KeyPressedEvent& e)
			{
				if (e.GetKeyCode() == Key::T)
					m_ActiveScene->DuplicateEntity(GetInspectorContext());
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
		
		ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_OpenOnArrow;
		
		if (m_Inspector.m_SelectedEntity == entity)
			flags |= ImGuiTreeNodeFlags_Selected;

		if (relationship.First == entt::null)
			flags |= ImGuiTreeNodeFlags_Leaf;

		bool opened = ImGui::TreeNodeEx((void*)(uint64_t)(uint32_t)entity, flags, entity.GetComponent<TagComponent>().Tag.c_str());

		if (ImGui::IsItemClicked())
			m_Inspector.SetSelectionContext(entity);

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
			return filepath.substr(pos + 7);
		}
		return std::string();
	}

	void EditorOverlay::DrawSceneSerializationPanel()
	{
		ImGui::Begin("Scene");

		ImGui::Dummy({ 0, 2.0f });
		ImGui::Text("Scene: %s", m_ActiveScene->m_SceneFilepath.c_str());
		ImGui::Dummy({ 0, 5.0f }); ImGui::Separator(); ImGui::Dummy({ 0, 5.0f });

		if (ImGui::Button("New scene", { 100, 25 }))
		{
			SceneManager::CreateNewEmptyScene("<Unsaved scene>");
			SceneManager::SetActiveScene("<Unsaved scene>");
		}
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

		bool saveAs = false;
		if (ImGui::Button("Save", { 100, 25 }))
			if (m_ActiveScene->m_SceneFilepath != "<Unsaved scene>")
				m_ActiveScene->SaveAsFile(SceneManager::GetActiveSceneFilepath());
			else
				saveAs = true;
		
		ImGui::SameLine();

		if (ImGui::Button("Save as", { 100, 25 }))
			saveAs = true;

		if (saveAs)
		{
			std::string sceneFile = GetSceneFilename(FileDialogs::SaveFile(".scene"));
			if (sceneFile.size())
				m_ActiveScene->SaveAsFile(sceneFile);
		}

		ImGui::Dummy({ 0, 5.0f }); ImGui::Separator(); ImGui::Dummy({ 0, 5.0f });

		ImGuiStyle& style = ImGui::GetStyle();

		float size = 75.0f;
		float avail = ImGui::GetContentRegionAvail().x;

		float off = (avail - size) * 0.5f;
		if (off > 0.0f)
			ImGui::SetCursorPosX(ImGui::GetCursorPosX() + off);

		if (m_ActiveScene->m_SceneState == SceneState::Play)
		{
			if (ImGui::Button("Stop", { 75, 30 }))
			{
				std::string filepath = m_ActiveScene->GetFilepath();
				if (filepath.size())
					m_ActiveScene->LoadFromFilepath(filepath);
			}
		}
		else
		{
			if (ImGui::Button("Play", { 75, 30 }))
				m_ActiveScene->OnBeginPlay();
		}
		
		ImGui::End();
	}

	void EditorOverlay::DrawPrefabPanel()
	{
		ImGui::Begin("Prefabs");

		if (ImGui::Button("Reload all"))
			PrefabManager::ReloadAllPrefabs();
		
		ImGui::Dummy({ 0.0f, 5.0f });
		for (auto& [tag, jsonData] : PrefabManager::s_Instance->m_PrefabsJsonData)
		{
			ImGui::Separator();
			ImGui::Text(tag.c_str());
			ImGui::SameLine(ImGui::GetWindowWidth() - 140);
			if (ImGui::Button(("Spawn##" + tag).c_str(), {60, 25}))
			{
				Entity entity = PrefabManager::SpawnPrefab(m_ActiveScene, tag);
				auto& transform = entity.GetComponent<TransformComponent>();
				glm::vec2 cameraPos = m_ActiveScene->GetPrimaryCameraPosition();
				transform.Position.x = cameraPos.x;
				transform.Position.y = cameraPos.y;
			}
			ImGui::SameLine();
			if (ImGui::Button(("Delete##" + tag).c_str(), {60, 25}))
			{
				PrefabManager::DeletePrefab(tag);
			}
		}
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
				scale.x *= (float)sprite.Sprite->m_Width_px / (float)sprite.Sprite->m_Height_px;
			}
			Renderer::SetLineWidth(glm::min(50.0f * padding, 1.0f));
			Renderer::DrawRect(transformMatrix, color);
		}
		Renderer::EndScene();
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
		io.DisplaySize = ImVec2((float)window.GetWidth(), (float)window.GetHeight());

		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			auto backupContext = glfwGetCurrentContext();
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault();
			glfwMakeContextCurrent(backupContext);
		}
	}

}
