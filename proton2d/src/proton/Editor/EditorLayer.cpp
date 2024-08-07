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
#include "Proton/Scripting/GameModeBase.h"
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
		: m_MainGameInstance(instance), m_GameInstanceContext(instance)
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

		ImGuiIO& io = ImGui::GetIO();
		io.ConfigFlags = ImGuiConfigFlags_DockingEnable | ImGuiConfigFlags_NavEnableKeyboard;

		SetupFonts();
		SetupImGuiViewports();
		SetupThemeStyle();

		// Initialize ImGui implementation for GLFW
		auto window = (GLFWwindow*)Application::Get().GetWindow().GetNativeWindow();
		ImGui_ImplGlfw_InitForOpenGL(window, true);
		ImGui_ImplOpenGL3_Init("#version 410");

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
		m_BlockEvents = s_Panels.SceneViewport.m_IsViewportHovered;

		for (auto& panel : m_EditorPanels)
			panel->OnUpdate(ts);

		for (auto& client : m_ClientInstances)
			client.Viewport->OnUpdate(ts);

		HandleClientGameInstanceCloseEvent();
	}

	void EditorLayer::OnImGuiRender()
	{
		ImGuiViewport* viewport = ImGui::GetMainViewport();

		ImGui::SetNextWindowPos(viewport->Pos);
		ImGui::SetNextWindowSize(viewport->Size);
		ImGui::SetNextWindowViewport(viewport->ID);

		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

		ImGuiWindowFlags windowFlags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar;
		windowFlags |= ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
		windowFlags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

		ImGui::Begin("DockSpace", NULL, windowFlags);
		ImGui::PopStyleVar(3);

		ImGuiIO& io = ImGui::GetIO();
		if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
		{
			ImGuiID dockspace_id = ImGui::GetID("DockSpace");
			ImGui::DockSpace(dockspace_id);
		}

		// Draw menu bar and editor panels
		m_MenuBar.OnImGuiRender();

		for (auto& panel : m_EditorPanels)
			panel->OnImGuiRender();

		for (auto& client : m_ClientInstances)
			client.Viewport->OnImGuiRender();

		ImGui::End();
	}

	void EditorLayer::OnEvent(Event& event)
	{
		ImGuiIO& io = ImGui::GetIO();
		auto viewport = GetSceneViewportPanel(m_GameInstanceContext);

		// Block keyboard events if typing in ImGui input fields
		// Block mouse events if viewport is not hovered by mouse, user is not dragging camera and ImGui wants to capture mouse
		if ((event.IsInCategory(EventCategoryKeyboard) && io.WantTextInput) ||
		    (event.IsInCategory(EventCategoryMouse) && io.WantCaptureMouse && !viewport->m_IsViewportHovered && !viewport->m_Camera->m_IsDragging))
		{
			event.Handled = true;
			return;
		}

		// Propagate event to other panels
		for (auto& panel : m_EditorPanels)
		{
			if (panel == &s_Panels.SceneViewport)
			{
				viewport->OnEvent(event);
				if (event.Handled)
					return;

				continue;
			}
			panel->OnEvent(event);
			if (event.Handled)
				return;
		}

		// Propagate event to game mode
		if (Scene* scene = m_GameInstanceContext->GetActiveScene())
		{
			GameModeBase* gameMode = scene->GetGameMode();
			gameMode->OnEvent(event);
		}
	}

	SceneViewportPanel* EditorLayer::GetSceneViewportPanel(GameInstance* gameInstance)
	{
		if (gameInstance && gameInstance != s_Instance->m_MainGameInstance)
		{
			return s_Instance->m_ClientViewportMap.at(gameInstance);
		}
		return &s_Panels.SceneViewport;
	}

	SceneViewportPanel* EditorLayer::GetSceneViewportPanel(Scene* scene)
	{
		return GetSceneViewportPanel(scene->m_GameInstance);
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
		if (!entity.IsValid()) // Handle unselect
		{
			Entity current = GetSelectedEntity();
			if (!current.IsValid())
				return;

			auto viewport = GetSceneViewportPanel(current.m_Scene->m_GameInstance);
			viewport->SetSelectedEntity(Entity{});
			return;
		}

		auto viewport = GetSceneViewportPanel(entity.m_Scene->m_GameInstance);
		viewport->SetSelectedEntity(entity);
	}

	Entity EditorLayer::GetSelectedEntity(bool mainInstanceOnly)
	{
		if (!mainInstanceOnly)
		{
			if (auto instance = GetFocusedGameInstance())
			{
				auto viewport = GetSceneViewportPanel(instance);
				return viewport->GetSelectedEntity();
			}
		}
		return s_Panels.SceneViewport.GetSelectedEntity();
	}

	Scene* EditorLayer::GetActiveScene(bool mainInstanceOnly)
	{
		if (!mainInstanceOnly)
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
		return s_Instance->m_GameInstanceContext;
	}

	SceneViewportPanel* EditorLayer::GetFocusedViewportPanel()
	{
		auto instance = GetFocusedGameInstance();
		return GetSceneViewportPanel(instance);
	}

	void EditorLayer::OnStartSimulationButton()
	{
		Scene* scene = GetActiveScene(true);
		scene->BeginPlay();
	}

	void EditorLayer::OnStopSimulationButton()
	{
		auto viewport = GetFocusedViewportPanel();
		if (viewport == &s_Panels.SceneViewport)
		{
			Scene* scene = GetActiveScene(true);
			if (m_MainGameInstance->GetNetMode() == NetMode::ListenServer)
			{
				m_ClientInstances.clear();
				m_CurrentInstanceID = 0;
			}
			scene->Stop();
		}
		else
		{
			auto instance = GetFocusedGameInstance();
			CloseClientGameInstance(instance->m_InstanceID);
		}
	}

	void EditorLayer::OnPauseSimulationButton()
	{
		Scene* scene = GetActiveScene();
		scene->Pause(!scene->IsPaused());
	}

	void EditorLayer::OnBeginSceneSimulation(Scene* scene)
	{
		if (!scene->m_GameInstance->IsMainInstance())
			return;

		m_SimulatedScenesBackup[scene->m_Filepath] = scene->CreateSceneCopy();

		SelectEntity(Entity{});
	}

	void EditorLayer::OnStopSceneSimulation(Scene* scene)
	{
		if (!scene->m_GameInstance->IsMainInstance())
			return;

		SelectEntity(Entity{});

		Scene* activeScene = GetActiveScene(true);
		bool isActiveScene = scene == activeScene;
		std::string sceneFilepath = scene->m_Filepath;

		// Restore scene state from backup
		SceneManager* manager = m_MainGameInstance->GetSceneManager();
		manager->m_Scenes[sceneFilepath] = m_SimulatedScenesBackup.at(sceneFilepath);
		m_SimulatedScenesBackup.erase(sceneFilepath);
			
		if (isActiveScene)
			manager->SetActiveScene(sceneFilepath);
	}

	GameInstance* EditorLayer::OpenNewClientGameInstance(NetMode netMode, bool loadStartupScene, const std::string& windowName)
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

		if (!loadStartupScene)
		{
			instance->Init(false);

			SceneManager* sceneManager = instance->GetSceneManager();
			Scene* activeScene = GetActiveScene(true);
			const std::string& filepath = activeScene->m_Filepath;

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
		m_GameInstanceContext = m_MainGameInstance;
		
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
		const auto& font = m_Config.EditorFonts["FiraCode"];
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
		style.TabBarOverlineSize = 0.0f;

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

		style.Colors[ImGuiCol_Tab] = ImVec4(0.12f, 0.12f, 0.12f, 1.0f);
		style.Colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.12f, 0.12f, 0.12f, 1.0f);
		style.Colors[ImGuiCol_TabHovered] = ImVec4(0.175f, 0.175f, 0.175f, 1.0f);
		style.Colors[ImGuiCol_TabActive] = ImVec4(0.175f, 0.175f, 0.175f, 1.0f);

		style.Colors[ImGuiCol_MenuBarBg] = ImVec4(0.1f, 0.1f, 0.1f, 1.0f);
	}

	void EditorLayer::SetupImGuiViewports()
	{
		ImGuiStyle& style = ImGui::GetStyle();
		ImGuiIO& io = ImGui::GetIO();

		// Enable viewports (ImGui windows can be detached from main application window)
		if (m_EnableViewports)
			io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

		if (m_EnableViewports && io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			style.WindowRounding = 0.0f;
			style.Colors[ImGuiCol_WindowBg].w = 1.0f;
		}
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
#endif // PT_EDITOR
