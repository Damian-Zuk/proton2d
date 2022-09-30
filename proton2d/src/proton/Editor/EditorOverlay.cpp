#include "pch.h"
#include "proton/Editor/EditorOverlay.h"
#include "proton/Core/Application.h"
#include "proton/Core/Window.h"

#include "proton/Editor/ImGui/imgui_impl_opengl3.h"
#include "proton/Editor/ImGui/imgui_impl_glfw.h"

#include <GLFW/glfw3.h>
#include <imgui.h>

namespace proton {

	void EditorOverlay::OnAttach()
	{
		ImGui::CreateContext();
		ImGui::StyleColorsDark();

		auto& io = ImGui::GetIO();
		io.ConfigFlags = ImGuiConfigFlags_DockingEnable | ImGuiConfigFlags_NavEnableKeyboard | ImGuiConfigFlags_ViewportsEnable;
		
		io.Fonts->AddFontFromFileTTF("assets/Roboto.ttf", 18);

		auto window = (GLFWwindow*)Application::Get().GetWindow().GetNativeWindow();
		ImGui_ImplGlfw_InitForOpenGL(window, true);
		ImGui_ImplOpenGL3_Init("#version 410");
	}

	void EditorOverlay::OnDetach()
	{
		ImGui_ImplOpenGL3_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();
	}

	void EditorOverlay::OnImGuiRender()
	{
		ImGui::ShowDemoWindow(); // Demo window for reference
		ImGui::Begin("Scene Entities");

		if (m_ActiveScene)
		{
			ImGui::Dummy({0.0f, 1.0f});
			if (ImGui::Button("Create new entity", {340.0f, 25.0f}))
			{
				m_ActiveScene->CreateEntity();
			}
			ImGui::Dummy({0.0f, 1.0f});

			m_ActiveScene->m_Registry.each([&](auto id)
			{
				Entity entity{ m_ActiveScene.get(), id};

				ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_OpenOnArrow;
				if (m_Inspector.m_SelectedEntity == entity)
					flags |= ImGuiTreeNodeFlags_Selected;

				bool expanded = ImGui::TreeNodeEx((void*)(uint64_t)(uint32_t)entity, 
					flags, entity.GetComponent<TagComponent>().Tag.c_str());
				
				// On select entity in scene entities panel
				if (ImGui::IsItemClicked()) 
				{
					m_Inspector.m_SelectedEntity = entity;
					// Reset inspector fields
					m_Inspector.m_SpriteComponentTextureSource.clear();
				}

				if (expanded)
				{
					ImGui::TreePop();
				}

			});

		}

		ImGui::End();

		m_Inspector.OnImGuiRender();
	}

	void EditorOverlay::SetActiveScene(const Shared<Scene>& scene)
	{
		m_ActiveScene = scene;
		m_Inspector.m_ActiveScene = scene;
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