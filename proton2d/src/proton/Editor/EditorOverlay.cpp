#include "pch.h"
#include "proton/Editor/EditorOverlay.h"
#include "proton/Core/Application.h"
#include "proton/Core/Window.h"
#include "proton/Scene/Components.h"
#include "proton/Assets/SceneSerializer.h"
#include "proton/Core/Utils.h"
#include "proton/Events/MouseEvents.h"

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
		io.Fonts->AddFontFromFileTTF(PROTON_ENGINE_ASSETS_DIR "Roboto.ttf", 18);

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
		m_CameraController.OnUpdate(ts);
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
			//   Other panels
			//************************************
			m_DebugInfo.m_EntitiesCount = m_ActiveScene->GetEntitiesCount();
			m_DebugInfo.m_ScriptedEntitiesCount = m_ActiveScene->GetScriptedEntitiesCount();

			m_Inspector.OnImGuiRender();
			m_DebugInfo.OnImGuiRender();
			DrawSceneSerializationPanel();
		}

		ImGui::End();
	}

	void EditorOverlay::OnEvent(Event& event)
	{
		m_CameraController.OnEvent(event);

		EventDispatcher dispatcher(event);
		dispatcher.Dispatch<MouseButtonPressedEvent>([&](MouseButtonPressedEvent& e)
		{
			// Select entity on mouse pressed event (Button 0)
			if (e.GetMouseButton() == Mouse::Button0 && !ImGui::GetIO().WantCaptureMouse)
			{
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

				SetInspectorContext(target);
			}
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

	void EditorOverlay::DrawSceneSerializationPanel()
	{
		static std::vector<std::string> directoryScenes = GetFilesFromDirectory("scenes");
		static int filenameIndex = -1;
		static std::string filename = directoryScenes.size() ? directoryScenes[0].c_str() : "No scenes found";
		static bool createNewScenePopup = false;

		ImGui::Begin("Scene##serialization_panel");

		// Level select combo
		if (ImGui::BeginCombo("##select_path", filenameIndex < 0 ? "Select scene..." :
			(directoryScenes.size() ? directoryScenes[filenameIndex].c_str() : "No scenes found")))
		{
			directoryScenes = GetFilesFromDirectory("scenes");

			if (ImGui::Selectable("Create new scene"))
				createNewScenePopup = true;

			ImGui::Separator();

			int index = 0;
			for (const auto& entry : directoryScenes)
			{
				if (ImGui::Selectable(entry.c_str(), filenameIndex == index))
					filenameIndex = index;

				if (filenameIndex == index)
					ImGui::SetItemDefaultFocus();
				index++;
			}
			ImGui::EndCombo();
		}

		if (createNewScenePopup)
		{
			ImGui::OpenPopup("Create new scene##popup");
			createNewScenePopup = false;
		}
		
		// Create new scene popup
		if (ImGui::BeginPopup("Create new scene##popup"))
		{
			static char sceneName[128] = { 0 };
			static bool keepContent = true;
			ImGui::Text("Create new scene");
			ImGui::Separator();
			ImGui::Text("Name");
			ImGui::SameLine();
			ImGui::PushItemWidth(200.0f);
			ImGui::InputText("##new_scene", sceneName, 128);
			ImGui::SameLine();

			if (ImGui::Button("Create##new_scene", { 75, 25 }) && strlen(sceneName))
			{
				std::string filepath = "scenes/" + std::string(sceneName) + ".json";
				
				if (!keepContent)
				{
					std::ofstream file(filepath);
					file << "{\"Scene name\": \"Sample scene\"}";
					file.close();
					m_ActiveScene->DestroyAll();
				}
				else
					SceneSerializer::Serialize(filepath);
					
				directoryScenes = GetFilesFromDirectory("scenes");
				int index = 0;
				for (const auto& entry : directoryScenes)
				{
					if (entry == sceneName)
						filenameIndex = index;
					index++;
				}
				ImGui::CloseCurrentPopup();
			}

			ImGui::Checkbox("Keep current scene content##new_scene", &keepContent);

			ImGui::EndPopup();
		}

		// Load / Save Buttons
		ImGui::Dummy({ 0, 3.0f });
		if (ImGui::Button("Load", { 75, 25 }) && filenameIndex >= 0)
			m_ActiveScene->LoadFromFilepath(directoryScenes[filenameIndex] + ".json");
		
		if (filenameIndex != -1)
		{
			ImGui::SameLine();
			if (ImGui::Button("Save", { 75, 25 }))
				m_ActiveScene->SaveAsFile(directoryScenes[filenameIndex] + ".json");
		}

		ImGui::Dummy({ 0.0f, 3.0f });
		if (m_ActiveScene->m_PlayState)
		{
			if (ImGui::Button("Stop", { 75, 25 }))
				m_ActiveScene->LoadFromFilepath(directoryScenes[filenameIndex] + ".json");
		}
		else
		{
			if (ImGui::Button("Play", { 75, 25 }))
			{
				m_ActiveScene->m_PlayState = 1;
				m_ActiveScene->OnBeginPlay();
			}
		}
		
		ImGui::End();
	}

	void EditorOverlay::SetSceneContext(Scene* context)
	{
	#ifndef PROTON_DISTRIBUTION
		s_Instance->m_ActiveScene = context;
		s_Instance->m_Inspector.m_ActiveScene = context;
		context->SetPrimaryCamera(s_Instance->m_CameraController.GetCamera());
		SceneSerializer::SetContext(context);
	#endif
	}

	void EditorOverlay::SetInspectorContext(Entity entity)
	{
	#ifndef PROTON_DISTRIBUTION
		s_Instance->m_Inspector.SetSelectionContext(entity);
	#endif
	}

	Entity EditorOverlay::GetInspectorContext()
	{
		return s_Instance->m_Inspector.m_SelectedEntity;
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
