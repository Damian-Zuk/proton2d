#include "pch.h"
#include "Proton/Editor/EditorLayer.h"
#include "Proton/Editor/Panels/MiscellaneousPanel.h"
#include "Proton/Editor/Panels/InspectorPanel.h"
#include "Proton/Editor/Panels/SceneHierarchyPanel.h"
#include "Proton/Editor/Panels/ScenePanel.h"
#include "Proton/Editor/Panels/PrefabPanel.h"

#include "Proton/Graphics/Renderer/Renderer.h"
#include "Proton/Core/Application.h"
#include "Proton/Core/Window.h"
#include "Proton/Utils/Utils.h"
#include "Proton/Events/MouseEvents.h"
#include "Proton/Events/KeyEvents.h"

#define IMGUI_IMPL_OPENGL_LOADER_GLAD
#include <backends/imgui_impl_opengl3.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.cpp>
#include <backends/imgui_impl_glfw.cpp>

#include <GLFW/glfw3.h>
#include <imgui.h>
#include <filesystem>


namespace proton {

	EditorLayer* EditorLayer::s_Instance = nullptr;

	EditorLayer::EditorLayer()
	{
		EditorLayer::s_Instance = this;
	}

	void EditorLayer::OnCreate()
	{
		// ImGui setup
		ImGui::CreateContext();
		ImGui::StyleColorsDark();

		auto& io = ImGui::GetIO();
		io.ConfigFlags = ImGuiConfigFlags_DockingEnable | ImGuiConfigFlags_NavEnableKeyboard;// | ImGuiConfigFlags_ViewportsEnable;
		io.Fonts->AddFontFromFileTTF("editor/content/font/Roboto.ttf", 18);

		ImGuiStyle& style = ImGui::GetStyle();
		style.FrameRounding = 7.0f;

		auto window = (GLFWwindow*)Application::Get().GetWindow().GetNativeWindow();
		ImGui_ImplGlfw_InitForOpenGL(window, true);
		ImGui_ImplOpenGL3_Init("#version 410");

		// Initialize editor panels
		PushEditorPanel("MiscellaneousPanel", new MiscellaneousPanel());
		PushEditorPanel("InspectorPanel", new InspectorPanel());
		PushEditorPanel("SceneHierarchy", new SceneHierarchyPanel());
		PushEditorPanel("ScenePanel", new ScenePanel());
		PushEditorPanel("PrefabPanel", new PrefabPanel());

		// Check if cache directory exist
		if (!std::filesystem::exists("editor/cache/"))
			if (std::filesystem::create_directories("editor/cache/"))
				PT_CORE_ERROR("[EditorLayer::OnCreate] Could not create cache directory!");
	}

	void EditorLayer::OnDestroy()
	{
		ImGui_ImplOpenGL3_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();

		for (auto& p : m_EditorPanels)
			delete p.second;
	}

	void EditorLayer::OnUpdate(float ts)
	{
		const glm::vec2& cursor = m_ActiveScene->GetCursorWorldPosition();

		// Update editor camera
		m_Camera.OnUpdate(ts);

		// Move selected entity
		if (m_MoveSelectedEntity && m_SelectedEntity.IsValid())
		{
			glm::vec2 targetPos = cursor + m_SelectionMouseOffset;
			auto& transform = m_SelectedEntity.GetComponent<TransformComponent>();
			transform.Position.x = targetPos.x;
			transform.Position.y = targetPos.y;
			ImGui::SetMouseCursor(7);
		}

		// Move editor camera
		if (m_MoveEditorCamera)
		{
			glm::vec2 offset = m_CameraDragOffset - cursor;
			m_Camera.m_Position.x += offset.x;
			m_Camera.m_Position.y += offset.y;
			ImGui::SetMouseCursor(2);
		}

		// Update editor panels
		for (auto& panel : m_EditorPanels)
			panel.second->OnUpdate(ts);
	}

	void EditorLayer::OnImGuiRender()
	{
		//ImGui::ShowDemoWindow(); // Demo window for reference
		for (auto& panel : m_EditorPanels) {
			panel.second->OnImGuiRender();
		}
		DrawCollidersAndSelectionOutline();
	}

	void EditorLayer::OnEvent(Event& event)
	{
		m_Camera.OnEvent(event);
		const glm::vec2& cursor = m_ActiveScene->GetCursorWorldPosition();
		EventDispatcher dispatcher(event);

		// Mouse events
		dispatcher.Dispatch<MouseButtonPressedEvent>([&](MouseButtonPressedEvent& e)
		{
			// Mouse Button 1 (Right): Move editor camera
			if (e.GetMouseButton() == Mouse::Button1 && !m_MoveEditorCamera)
			{
				m_CameraDragOffset = cursor;
				m_MoveEditorCamera = true;
			}

			// Mouse Button 0 (Left): Select Entity
			else if (e.GetMouseButton() == Mouse::Button0)
			{
				Entity target; float transformMaxZ = 0.0f;

				for (auto& entity : m_ActiveScene->GetEntitiesOnCursorLocation())
				{
					auto& transform = entity.GetComponent<TransformComponent>();
					if (!target || transform.Position.z > transformMaxZ)
					{
						target = entity;
						transformMaxZ = transform.Position.z;
					}
				}

				if (m_SelectedEntity && m_ActiveScene->IsCursorHoveringEntity(m_SelectedEntity))
				{
					auto& transform = m_SelectedEntity.GetComponent<TransformComponent>();
					auto& targetTransform = target.GetComponent<TransformComponent>();
					if (transform.Scale.x < targetTransform.Scale.x
						&& transform.Scale.y < targetTransform.Scale.y)
					{
						// Discard selection
						target = m_SelectedEntity;
					}	
				}

				if (target && target == m_SelectedEntity)
				{
					m_MoveSelectedEntity = true;
					auto& transform = m_SelectedEntity.GetComponent<TransformComponent>();
					m_SelectionMouseOffset = glm::vec2{ transform.Position.x, transform.Position.y } - cursor;
				}

				SelectEntity(target);
			}

			return false;
		});

		// Keyboard events
		dispatcher.Dispatch<KeyPressedEvent>([&](KeyPressedEvent& e)
		{
			KeyCode key = e.GetKeyCode();

			if (key == Key::F1)
				m_ShowSelectionOutline = !m_ShowSelectionOutline;
			
			if (key == Key::F2)
				m_ShowSelectionCollider = !m_ShowSelectionCollider;
			
			if (key == Key::F3)
				m_ShowAllColliders = !m_ShowAllColliders;

			if (key == Key::Escape && m_SelectedEntity)
				SelectEntity({});

			if (key == Key::Delete && m_SelectedEntity)
			{
				m_SelectedEntity.Destroy();
				SelectEntity({});
			}

			return false;
		});

		// Mouse evenets (button released)
		dispatcher.Dispatch<MouseButtonReleasedEvent>([&](MouseButtonReleasedEvent& e)
		{
			if (e.GetMouseButton() == Mouse::Button0)
				m_MoveSelectedEntity = false;
			if (e.GetMouseButton() == Mouse::Button1)
				m_MoveEditorCamera = false;

			ImGui::SetMouseCursor(0);
			return false;
		});
	}

	void EditorLayer::DrawCollidersAndSelectionOutline()
	{
		Renderer::BeginScene(*m_ActiveScene->GetPrimaryCamera(), m_ActiveScene->GetPrimaryCameraPosition());
		// Draw box colliders
		auto view = m_ActiveScene->m_Registry.view<TransformComponent, BoxColliderComponent>();
		for (auto entity : view)
		{
			auto [transform, bc] = view.get<TransformComponent, BoxColliderComponent>(entity);

			// Check if current entity is selected and draw collider rect
			bool drawSelected = m_ShowSelectionCollider && m_SelectedEntity.m_Handle == entity;

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
		if (m_SelectedEntity.IsValid() && m_ShowSelectionOutline)
		{
			auto& transform = m_SelectedEntity.GetComponent<TransformComponent>();
			float padding = glm::sqrt(m_ActiveScene->GetPrimaryCamera()->GetZoomLevel()) * 0.05f;
			glm::vec3 position = { transform.Position.x, transform.Position.y, 0.21f };
			glm::vec3 scale = { transform.Scale.x + padding, transform.Scale.y + padding, 1.0f };
			glm::mat4 transformMatrix = Math::GetTransform(position, scale, transform.Rotation);

			glm::vec4 color = m_ShowSelectionOutline && m_MoveSelectedEntity
				? glm::vec4{ 0.8f, 0.8f, 0.2f, 1.0f } : glm::vec4{ 1.0f };

			if (m_SelectedEntity.HasComponent<SpriteComponent>())
			{
				auto& sprite = m_SelectedEntity.GetComponent<SpriteComponent>();
				if (sprite.Sprite)
					scale.x *= (float)sprite.Sprite.m_PixelSize.x / (float)sprite.Sprite.m_PixelSize.y;
			}
			Renderer::SetLineWidth(glm::min(50.0f * padding, 1.0f));
			Renderer::DrawRect(transformMatrix, color);
		}
		Renderer::EndScene();
	}

	void EditorLayer::ResetCameraPosition()
	{
		m_Camera.m_Position = { 0.0f, 0.0f, 0.0f };
	}

	void EditorLayer::SetActiveScene(Scene* scene)
	{
		s_Instance->m_ActiveScene = scene;
		for (auto& panel : s_Instance->m_EditorPanels)
		{
			panel.second->m_ActiveScene = scene;
		}
	}

	void EditorLayer::SelectEntity(Entity entity)
	{
		s_Instance->m_SelectedEntity = entity;
		for (auto& panel : s_Instance->m_EditorPanels)
			panel.second->m_SelectedEntity = entity;
	}

	void EditorLayer::PushEditorPanel(const std::string& name, EditorPanel* panel)
	{
		m_EditorPanels.push_back(std::make_pair(name, panel));
		panel->m_ActiveScene = m_ActiveScene;
	}

	void EditorLayer::PopEditorPanel(const std::string& name)
	{
		m_EditorPanels.erase(std::remove_if(m_EditorPanels.begin(), m_EditorPanels.end(),
			[&name](const std::pair<std::string, EditorPanel*>& element) {
				return element.first == name;
			}),
			m_EditorPanels.end());
	}

	void EditorLayer::BeginImGuiRender()
	{
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();
	}

	void EditorLayer::EndImGuiRender()
	{
		auto& io = ImGui::GetIO();
		auto& window = Application::Get().GetWindow();
		io.DisplaySize = { (float)window.GetWidth(), (float)window.GetHeight() };

		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
	}

}
