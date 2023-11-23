#include "pch.h"
#include "proton/Editor/EditorLayer.h"
#include "proton/Editor/Panels/MiscellaneousPanel.h"
#include "proton/Editor/Panels/InspectorPanel.h"
#include "proton/Editor/Panels/SceneHierarchyPanel.h"
#include "proton/Editor/Panels/ScenePanel.h"
#include "proton/Editor/Panels/PrefabPanel.h"

#include "proton/Graphics/Renderer/Renderer.h"
#include "proton/Core/Application.h"
#include "proton/Core/Window.h"
#include "proton/Utils/Utils.h"
#include "proton/Events/MouseEvents.h"
#include "proton/Events/KeyEvents.h"

#define IMGUI_IMPL_OPENGL_LOADER_GLAD
#include <backends/imgui_impl_opengl3.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.cpp>
#include <backends/imgui_impl_glfw.cpp>

#include <GLFW/glfw3.h>
#include <imgui.h>


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
		io.Fonts->AddFontFromFileTTF("assets/Roboto.ttf", 18);

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
	}


	void EditorLayer::OnDestroy()
	{
		ImGui_ImplOpenGL3_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();

		for (auto& panel : m_EditorPanels)
			delete panel.second;
	}


	void EditorLayer::OnUpdate(float ts)
	{
		// Update editor camera
		m_Camera.OnUpdate(ts);

		// Move selected entity
		if (m_MoveSelectedEntity && m_SelectedEntity.IsValid())
		{
			glm::vec2 targetPos = m_ActiveScene->GetCursorPosition() + m_SelectionMouseOffset;
			auto& transform = m_SelectedEntity.GetComponent<TransformComponent>();
			transform.Position = { targetPos.x, targetPos.y, transform.Position.z };
			ImGui::SetMouseCursor(7);
		}

		// Move editor camera
		if (m_MoveEditorCamera)
		{
			glm::vec2 mousePos = m_ActiveScene->GetCursorPosition();
			glm::vec2 offset = m_CameraDragOffset - mousePos;
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
		EventDispatcher dispatcher(event);

		// Mouse events
		dispatcher.Dispatch<MouseButtonPressedEvent>([&](MouseButtonPressedEvent& e)
		{
			if (ImGui::GetIO().WantCaptureMouse)
				return false;

			// Mouse Button 1 (Right): Move editor camera
			if (e.GetMouseButton() == Mouse::Button1 && !m_MoveEditorCamera)
			{
				m_CameraDragOffset = m_ActiveScene->GetCursorPosition();
				m_MoveEditorCamera = true;
			}

			// Mouse Button 0 (Left: Select Entity
			else if (e.GetMouseButton() == Mouse::Button0)
			{
				glm::vec2 mousePos = m_ActiveScene->GetCursorPosition();
				Entity& selectedEntity = m_SelectedEntity;

				std::vector<Entity> entities = m_ActiveScene->GetCursorEntities();
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

				if (selectedEntity && m_ActiveScene->IsCursorOverEntity(selectedEntity))
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
					m_MoveSelectedEntity = true;
					auto& transform = selectedEntity.GetComponent<TransformComponent>();
					m_SelectionMouseOffset = glm::vec2{ transform.Position.x, transform.Position.y } - mousePos;
				}

				SelectEntity(target);
			}

			return true;
		});

		// Keyboard events
		dispatcher.Dispatch<KeyPressedEvent>([&](KeyPressedEvent& e)
		{
			if (ImGui::GetIO().WantTextInput)
				return false;

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

			return true;
		});

		// Mouse evenets (button released)
		dispatcher.Dispatch<MouseButtonReleasedEvent>([&](MouseButtonReleasedEvent& e)
		{
			if (e.GetMouseButton() == Mouse::Button0)
				m_MoveSelectedEntity = false;
			if (e.GetMouseButton() == Mouse::Button1)
				m_MoveEditorCamera = false;

			ImGui::SetMouseCursor(0);
			return true;
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
