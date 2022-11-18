#include "pch.h"
#include "proton/Editor/EditorOverlay.h"
#include "proton/Core/Application.h"
#include "proton/Core/Window.h"
#include "proton/Entity/Components.h"
#include "proton/Assets/SceneSerializer.h"

#define IMGUI_IMPL_OPENGL_LOADER_GLAD
#include <backends/imgui_impl_opengl3.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.cpp>
#include <backends/imgui_impl_glfw.cpp>

#include <GLFW/glfw3.h>
#include <imgui.h>
#include <filesystem>
#include <fstream>

namespace proton {

	EditorOverlay* EditorOverlay::s_Instance = nullptr;

	EditorOverlay::EditorOverlay()
	{
		EditorOverlay::s_Instance = this;
	}

	void EditorOverlay::OnCreate()
	{
		ImGui::CreateContext();
		ImGui::StyleColorsDark();

		auto& io = ImGui::GetIO();
		io.ConfigFlags = ImGuiConfigFlags_DockingEnable | ImGuiConfigFlags_NavEnableKeyboard;// | ImGuiConfigFlags_ViewportsEnable;
		
		io.Fonts->AddFontFromFileTTF("assets/Roboto.ttf", 18);

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

	static std::vector<std::string> GetScenesFromDirectory()
	{
		std::vector<std::string> directoryScenes;
		for (const auto& entry : std::filesystem::directory_iterator("scenes"))
			directoryScenes.emplace_back(entry.path().stem().generic_string());
		return directoryScenes;
	}

	void EditorOverlay::DrawSceneSerializationPanel()
	{
		static std::vector<std::string> directoryScenes = GetScenesFromDirectory();
		static std::string filename = directoryScenes.size() ? directoryScenes[0].c_str() : "No scenes found";
		static int filenameIndex = 0;
		static bool comboPlaceholder = true;
		static bool createNewScenePopup = false;

		ImGui::Begin("Scene##serialization_panel");

		if (ImGui::BeginCombo("##select_path", comboPlaceholder ? "Select scene..." : 
			(directoryScenes.size() ? directoryScenes[filenameIndex].c_str() : "No scenes found")))
		{
			comboPlaceholder = false;
			directoryScenes = GetScenesFromDirectory();

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
		
		if (ImGui::BeginPopup("Create new scene##popup"))
		{
			static char sceneName[128] = { 0 };
			static bool emptyScene = true;
			ImGui::Text("Create new scene");
			ImGui::Separator();
			ImGui::Text("Name");
			ImGui::SameLine();
			ImGui::PushItemWidth(200.0f);
			ImGui::InputText("##new_scene", sceneName, 128);
			ImGui::SameLine();

			if (ImGui::Button("Create##new_scene", { 75, 25 }))
			{
				std::string filepath = "scenes/" + std::string(sceneName) + ".json";
				
				if (emptyScene)
				{
					std::ofstream file(filepath);
					file << "{\"Scene name\": \"Sample scene\"}";
					file.close();
					m_ActiveScene->DestroyAllEntities();
				}
				else
					SceneSerializer::Serialize(filepath);
					
				directoryScenes = GetScenesFromDirectory();
				int index = 0;
				for (const auto& entry : directoryScenes)
				{
					if (entry == sceneName)
						filenameIndex = index;
					index++;
				}
				ImGui::CloseCurrentPopup();
			}

			ImGui::Checkbox("Empty scene##new_scene", &emptyScene);

			ImGui::EndPopup();
		}

		if (ImGui::Button("Load", { 75, 25 }))
		{
			m_ActiveScene->DestroyAllEntities();
			SceneSerializer::Deserialize("scenes/" + directoryScenes[filenameIndex] + ".json");
		}
		ImGui::SameLine();

		if (ImGui::Button("Save", { 75, 25 }))
			SceneSerializer::Serialize("scenes/" + directoryScenes[filenameIndex] + ".json");

		ImGui::End();
	}

	void EditorOverlay::SetSceneContext(Scene* context)
	{
		s_Instance->m_ActiveScene = context;
		s_Instance->m_Inspector.m_ActiveScene = context;
		context->SetPrimaryCamera(s_Instance->m_CameraController.GetCamera());
		SceneSerializer::SetContext(context);
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