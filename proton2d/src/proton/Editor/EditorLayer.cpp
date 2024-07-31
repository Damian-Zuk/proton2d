#include "ptpch.h"
#ifdef PT_EDITOR
#include "Proton/Editor/EditorLayer.h"
#include "Proton/Editor/Panels/SceneViewportPanel.h"
#include "Proton/Editor/Panels/InspectorPanel.h"
#include "Proton/Editor/Panels/SceneHierarchyPanel.h"
#include "Proton/Editor/Panels/ToolbarPanel.h"
#include "Proton/Editor/Panels/ContentBrowserPanel.h"
#include "Proton/Editor/Panels/SettingsPanel.h"
#include "Proton/Editor/Panels/InfoPanel.h"

#include "Proton/Core/Application.h"
#include "Proton/Core/GameInstance.h"
#include "Proton/Core/Window.h"
#include "Proton/Graphics/Renderer/Renderer.h"
#include "Proton/Graphics/Renderer/Framebuffer.h"
#include "Proton/Events/KeyEvents.h"
#include "Proton/Events/MouseEvents.h"
#include "Proton/Utils/Utils.h"
#include "Proton/Scene/SceneManager.h"
#include "Proton/Physics/PhysicsWorld.h"

#define IMGUI_IMPL_OPENGL_LOADER_GLAD
#include <backends/imgui_impl_opengl3.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.cpp>
#include <backends/imgui_impl_glfw.cpp>

#include <imgui.h>
#include <glm/gtc/type_ptr.hpp>

namespace proton {

	EditorLayer* EditorLayer::s_Instance = nullptr;

	struct EditorPanels
	{
		SettingsPanel Settings;
		InfoPanel Info;
		InspectorPanel Inspector;
		SceneHierarchyPanel SceneHiearchy;
		ToolbarPanel Toolbar;
		ContentBrowserPanel ContentBrowser;
		SceneViewportPanel SceneViewport;

	} static s_Panels;

	struct EditorFonts
	{
		ImFont* FontAwesome;
		ImFont* SmallFont;

	} static s_Fonts;

	EditorLayer::EditorLayer(GameInstance* instance)
		: m_MainGameInstance(instance), m_FocusedGameInstance(instance)
	{
	}

	EditorLayer* EditorLayer::Get()
	{
		return s_Instance;
	}

	void EditorLayer::OnCreate()
	{
		// ImGui setup
		ImGui::CreateContext();
		ImGui::StyleColorsDark();

		SetupFonts();
		SetupImGuiViewports();
		SetupThemeStyle();

		// Initialize ImGui implementation for GLFW
		InitializeImGui();

		// Initialize editor panels
		m_EditorPanels.push_back(&s_Panels.Settings);
		m_EditorPanels.push_back(&s_Panels.Info);
		m_EditorPanels.push_back(&s_Panels.Inspector);
		m_EditorPanels.push_back(&s_Panels.SceneHiearchy);
		m_EditorPanels.push_back(&s_Panels.Toolbar);
		m_EditorPanels.push_back(&s_Panels.ContentBrowser);
		m_EditorPanels.push_back(&s_Panels.SceneViewport);

		s_Panels.SceneViewport.m_GameInstance = m_MainGameInstance;

		for (EditorPanel* panel : m_EditorPanels)
			panel->OnCreate();
	}

	void EditorLayer::OnDestroy()
	{
		ImGui_ImplOpenGL3_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();
	}

	void EditorLayer::OnUpdate(float ts)
	{
		for (auto& panel : m_EditorPanels)
			panel->OnUpdate(ts);

		for (auto& client : m_ClientInstances)
			client.Viewport->OnUpdate(ts);

		HandleClientGameInstanceCloseEvent();
	}

	void EditorLayer::OnImGuiRender()
	{
		//ImGui::ShowDemoWindow(); // Demo window for reference
		
		// DockSpace window flags
		ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
		window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
		window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

		ImGui::PushStyleVar(ImGuiStyleVar_TabBarBorderSize, 0.0f);

		ImGuiViewport* viewport = ImGui::GetMainViewport();
		ImGui::SetNextWindowPos(viewport->Pos);
		ImGui::SetNextWindowSize(viewport->Size);
		ImGui::SetNextWindowViewport(viewport->ID);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

		static bool dockspaceOpen = true;
		ImGui::Begin("DockSpace", &dockspaceOpen, window_flags);
		ImGui::PopStyleVar(3);

		// DockSpace
		ImGuiIO& io = ImGui::GetIO();
		ImGuiStyle& style = ImGui::GetStyle();
		float minWinSizeX = style.WindowMinSize.x;
		style.WindowMinSize.x = 370.0f;
		if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
		{
			ImGuiID dockspace_id = ImGui::GetID("DockSpace");
			ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);
		}
		style.WindowMinSize.x = minWinSizeX;

		// Render editor panels
		m_MenuBar.OnImGuiRender();

		for (auto& panel : m_EditorPanels)
			panel->OnImGuiRender();

		for (auto& client : m_ClientInstances)
			client.Viewport->OnImGuiRender();

		ImGui::End(); // DockSpace
		ImGui::PopStyleVar();
	}

	void EditorLayer::OnEvent(Event& event)
	{
		ImGuiIO& io = ImGui::GetIO();

		// Block events if viewport is not hovered by mouse
		if (m_BlockEvents)
		{
			event.Handled |= event.IsInCategory(EventCategoryMouse) & io.WantCaptureMouse;
			event.Handled |= event.IsInCategory(EventCategoryKeyboard) & io.WantCaptureKeyboard;
			if (event.Handled)
				return;
		}
		else if (event.IsInCategory(EventCategoryKeyboard) && io.WantTextInput)
		{
			event.Handled = true;
			return;
		}

		for (auto& panel : m_EditorPanels)
			panel->OnEvent(event);

		for (const auto& instance : m_ClientInstances)
			instance.Viewport->OnEvent(event);
	}

	SceneViewportPanel* EditorLayer::GetSceneViewportPanel(GameInstance* gameInstance)
	{
		if (gameInstance && gameInstance != s_Instance->m_MainGameInstance)
		{
			return s_Instance->m_ClientViewportMap.at(gameInstance);
		}
		return &s_Panels.SceneViewport;
	}

	SceneHierarchyPanel* EditorLayer::GetSceneHierarchyPanel()
	{
		return &s_Panels.SceneHiearchy;
	}

	InspectorPanel* EditorLayer::GetInspectorPanel()
	{
		return &s_Panels.Inspector;
	}

	void EditorLayer::SetActiveScene(Scene* scene)
	{
		auto viewport = GetSceneViewportPanel(scene->m_GameInstance);
		viewport->SetActiveScene(scene);
	}

	void EditorLayer::SelectEntity(Entity entity)
	{
		if (!entity.IsValid())
		{
			Entity current = GetSelectedEntity();

			if (!current)
				return;

			auto viewport = GetSceneViewportPanel(current.m_Scene->m_GameInstance);
			viewport->SetSelectedEntity(entity);
			return;
		}

		auto viewport = GetSceneViewportPanel(entity.m_Scene->m_GameInstance);
		viewport->SetSelectedEntity(entity);
	}

	Entity EditorLayer::GetSelectedEntity(bool targetFocusedViewport)
	{
		if (targetFocusedViewport)
		{
			if (auto instance = GetFocusedGameInstance())
			{
				auto viewport = GetSceneViewportPanel(instance);
				return viewport->GetSelectedEntity();
			}
		}
		return s_Panels.SceneViewport.GetSelectedEntity();
	}

	Scene* EditorLayer::GetActiveScene(bool targetFocusedViewport)
	{
		if (targetFocusedViewport)
		{
			if (auto instance = GetFocusedGameInstance())
			{
				return instance->GetActiveScene();
			}
		}
		return s_Instance->m_MainGameInstance->GetActiveScene();
	}

	GameInstance* EditorLayer::GetFocusedGameInstance()
	{
		return s_Instance->m_FocusedGameInstance;
	}

	SceneViewportPanel* EditorLayer::GetFocusedViewportPanel()
	{
		auto instance = GetFocusedGameInstance();
		return GetSceneViewportPanel(instance);
	}

	void EditorLayer::OnStartSimulationButton()
	{
		Scene* activeScene = GetActiveScene(false);
		activeScene->BeginPlay();
	}

	void EditorLayer::OnStopSimulationButton()
	{
		auto viewport = GetFocusedViewportPanel();
		if (viewport == &s_Panels.SceneViewport)
		{
			Scene* activeScene = GetActiveScene(false);
			if (m_MainGameInstance->GetNetMode() == NetMode::ListenServer)
			{
				m_ClientInstances.clear();
				m_CurrentInstanceID = 0;
			}
			activeScene->Stop();
		}
		else
		{
			auto instance = GetFocusedGameInstance();
			CloseClientGameInstance(instance->m_InstanceID);
		}
	}

	void EditorLayer::OnPauseSimulationButton()
	{
		Scene* activeScene = GetActiveScene();
		activeScene->Pause(!activeScene->IsPaused());
	}

	void EditorLayer::OnBeginSceneSimulation(Scene* scene)
	{
		if (!scene->m_GameInstance->IsMainInstance())
			return;

		m_SimulatedScenesBackup[scene->m_SceneFilepath] = scene->CreateSceneCopy();
		m_SimulatedScenes++;

		SelectEntity(Entity{});
	}

	void EditorLayer::OnStopSceneSimulation(Scene* scene)
	{
		if (!scene->m_GameInstance->IsMainInstance())
			return;

		Scene* activeScene = GetActiveScene(false);
		bool isActiveScene = scene == activeScene;
		std::string sceneFilepath = scene->m_SceneFilepath;

		SceneManager* manager = m_MainGameInstance->GetSceneManager();
		manager->m_Scenes[sceneFilepath] = m_SimulatedScenesBackup.at(sceneFilepath);
			
		if (isActiveScene)
			manager->SetActiveScene(sceneFilepath);
			
		m_SimulatedScenesBackup.erase(sceneFilepath);
		m_SimulatedScenes--;
	}

	GameInstance* EditorLayer::OpenNewClientGameInstance(NetMode netMode, bool currentSceneStartup, const std::string& windowName)
	{
		uint32_t id = ++m_CurrentInstanceID;

		m_ClientInstances.push_back(EditorClientInstance{
			MakeUnique<GameInstance>(),
			MakeUnique<SceneViewportPanel>(), id
		});

		auto& client = m_ClientInstances.back();
		auto instance = client.Instance.get();
		auto viewport = client.Viewport.get();

		m_ClientViewportMap[instance] = viewport;
		
		instance->m_IsMainInstance = false;
		instance->m_InstanceID = id;
		instance->SetNetMode(netMode);

		viewport->m_ImGuiWindowName = windowName;
		viewport->m_GameInstance = instance;
		viewport->m_IsMainViewport = false;
		viewport->OnCreate();

		if (currentSceneStartup)
		{
			instance->Init(false);

			SceneManager* sceneManager = instance->GetSceneManager();
			Scene* activeScene = GetActiveScene(false);
			const std::string& filepath = activeScene->m_SceneFilepath;

			// Copy currently active scene to a new game instance
			sceneManager->AddNewActiveScene(filepath, activeScene->IsSimulated() ?
				m_SimulatedScenesBackup.at(filepath)->CreateSceneCopy(instance)
				: activeScene->CreateSceneCopy(instance));
		}
		else
		{
			instance->Init();
		}

		return instance;
	}

	void EditorLayer::CloseClientGameInstance(uint32_t instanceID)
	{
		m_ClientInstancesToClose.push_back(instanceID);

		if (m_ClientInstances.size() == 1)
			m_CurrentInstanceID = 0;
	}

	void EditorLayer::HandleClientGameInstanceCloseEvent()
	{
		if (!m_ClientInstancesToClose.size())
			return;

		for (uint32_t id : m_ClientInstancesToClose)
		{
			// Destroy Client GameInstance and SceneViewportPanel
			m_ClientInstances.erase(std::remove_if(m_ClientInstances.begin(), m_ClientInstances.end(),
				[id](const EditorClientInstance& instance) {
					return instance.ID == id;
				}
			));
		}

		m_ClientInstancesToClose.clear();
		m_FocusedGameInstance = m_MainGameInstance;
		
		auto viewport = GetSceneViewportPanel();
		viewport->m_IsViewportFocused = true;
	}

	EditorCamera* EditorLayer::GetCamera()
	{
		return s_Panels.SceneViewport.m_Camera.get();
	}

	ImFont* EditorLayer::GetFontAwesome()
	{
		return s_Fonts.FontAwesome;
	}

	ImFont* EditorLayer::GetSmallFont()
	{
		return s_Fonts.SmallFont;
	}

	void EditorLayer::SetupFonts()
	{
		ImGuiIO& io = ImGui::GetIO();
		ImGuiStyle& style = ImGui::GetStyle();

		// Main font
		const EditorConfig::Font font = m_Config.EditorFonts["roboto"];
		io.Fonts->AddFontFromFileTTF(font.FontFilepath.c_str(), font.FontSize);

		// Small font
		s_Fonts.SmallFont = io.Fonts->AddFontFromFileTTF(font.FontFilepath.c_str(), 14.0f);

		// Font Awesome
		static const ImWchar icons_ranges[] = { 0xE000, 0xF8FF, 0 };
		s_Fonts.FontAwesome = io.Fonts->AddFontFromFileTTF("editor/content/font/FontAwesome.ttf", 18.0f, NULL, icons_ranges);
	}

	void EditorLayer::SetupThemeStyle()
	{
		// Styles
		ImGuiStyle& style = ImGui::GetStyle();
		ImGuiIO& io = ImGui::GetIO();

		style.FrameRounding = 6.0f;
		style.PopupRounding = 7.0f;
		style.ScrollbarSize = 20.0f;
		style.WindowBorderSize = 0.0f;

		// Color styles
		style.Colors[ImGuiCol_Border] = ImVec4(0.18f, 0.18f, 0.18f, 1.0f);
		style.Colors[ImGuiCol_FrameBg] = ImVec4(0.18f, 0.18f, 0.18f, 1.0f);

		style.Colors[ImGuiCol_Button] = ImVec4(0.4f, 0.18f, 0.19f, 1.0f);
		style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.5f, 0.18f, 0.19f, 1.0f);
		style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.6f, 0.18f, 0.19f, 1.0f);

		style.Colors[ImGuiCol_SliderGrab] = ImVec4(0.4f, 0.18f, 0.19f, 1.0f);
		style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.6f, 0.18f, 0.19f, 1.0f);

		style.Colors[ImGuiCol_WindowBg] = ImVec4(0.1f, 0.1f, 0.1f, 0.7f);
		style.Colors[ImGuiCol_CheckMark] = ImVec4(0.0f, 1.0f, 0.0f, 1.0f);

		style.Colors[ImGuiCol_Header] = ImVec4(0.32f, 0.32f, 0.32f, 1.0f);
		style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.32f, 0.32f, 0.32f, 1.0f);
		style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.32f, 0.32f, 0.32f, 1.0f);

		style.Colors[ImGuiCol_TitleBg] = ImVec4(0.23f, 0.23f, 0.22f, 1.0f);
		style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.23f, 0.23f, 0.22f, 1.0f);

		style.Colors[ImGuiCol_Tab] = ImVec4(0.18f, 0.18f, 0.18f, 1.0f);
		style.Colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.18f, 0.18f, 0.18f, 1.0f);
		style.Colors[ImGuiCol_TabHovered] = ImVec4(0.4f, 0.4f, 0.4f, 1.0f);
		style.Colors[ImGuiCol_TabActive] = ImVec4(0.4f, 0.4f, 0.4f, 1.0f);
	}

	void EditorLayer::SetupImGuiViewports()
	{
		ImGuiIO& io = ImGui::GetIO();
		ImGuiStyle& style = ImGui::GetStyle();

		io.ConfigFlags = ImGuiConfigFlags_DockingEnable | ImGuiConfigFlags_NavEnableKeyboard;
		// Enable viewports (ImGui windows can be detached from main application window)
		if (m_EnableViewports)
			io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

		if (m_EnableViewports && io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			style.WindowRounding = 0.0f;
			style.Colors[ImGuiCol_WindowBg].w = 1.0f;
		}
	}

	void EditorLayer::InitializeImGui()
	{
		auto window = (GLFWwindow*)Application::Get().GetWindow().GetNativeWindow();
		ImGui_ImplGlfw_InitForOpenGL(window, true);
		ImGui_ImplOpenGL3_Init("#version 410");
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

	#ifdef PROTON_PLATFORM_WINDOWS
		if (m_EnableViewports && io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			GLFWwindow* backup_current_context = glfwGetCurrentContext();
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault();
			glfwMakeContextCurrent(backup_current_context);
		}
	#endif
	}

}
#endif
