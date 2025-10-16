#include "ptpch.h"
#include "Proton/Core/Application.h"
#include "Proton/Core/GameInstance.h"
#include "Proton/Core/Timer.h"
#include "Proton/Core/Input.h"
#include "Proton/Core/AssetManager.h"

#include "Proton/Events/WindowEvents.h" 
#include "Proton/Events/KeyEvents.h"
#include "Proton/Events/MouseEvents.h"

#include "Proton/Graphics/Renderer/Renderer.h"
#include "Proton/Scene/SceneManager.h"
#include "Proton/Scene/PrefabManager.h"
#include "Proton/Scripting/GameModeBase.h"

#ifdef PROTON_PLATFORM_WINDOWS
#include "Proton/Platform/Windows/WindowsWindow.h"
#endif

#ifdef PT_EDITOR
#include "Proton/Editor/EditorLayer.h"
#include "Proton/Editor/Panels/SceneViewportPanel.h"
#include "Proton/Editor/Menu/EditorMenuBar.h"
#endif

namespace proton {

	Application* Application::s_Instance = nullptr;

	Application::Application()
	{
		PT_CORE_ASSERT(!s_Instance, "Application already exists!");
		Application::s_Instance = this;

		m_AppConfig.LoadConfig();

		WindowSpecification windowSpec;
		windowSpec.Title = m_AppConfig.WindowTitle;
		windowSpec.Width = m_AppConfig.WindowWidth;
		windowSpec.Height = m_AppConfig.WindowHeight;
		windowSpec.Fullscreen = m_AppConfig.Fullscreen;
		windowSpec.VSync = m_AppConfig.VSync;

	#ifdef PROTON_PLATFORM_WINDOWS
		m_Window = MakeUnique<WindowsWindow>(windowSpec);
	#endif

		m_Window->SetEventCallback(PT_BIND_FUNCTION(Application::OnEvent));

	#ifdef PROTON_DISTRIBUTION
		m_GameInstance = MakeUnique<GameInstance>();
	#endif
	}

	Application::~Application()
	{
		for (AppLayer* layer : m_AppLayers)
		{
			layer->OnDestroy();
			delete layer;
		}
		Renderer::Shutdown();
	}

	void Application::Run()
	{
		if (m_IsRunning)
		{
			PT_CORE_ERROR("Application is already running!");
			return;
		}

		AssetManager::Init();
		Renderer::Init();
		PrefabManager::Init();

	#ifdef PT_EDITOR
		EditorLayer::s_Instance = new EditorLayer();
		PushOverlay(EditorLayer::s_Instance); // EditorLayer::OnCreate()
	#endif

		if (!OnCreate())
		{
			PT_CORE_ERROR("Application initialization failed! Exiting.");
			return;
		}

		m_IsRunning = true;

	#ifdef PROTON_DISTRIBUTION
		m_GameInstance->Init();
		Renderer::SetViewport(0, 0, m_Window->GetWidth(), m_Window->GetHeight());
	#endif

		PROFILE_BEGIN_SESSION("Proton-Runtime");
		PROFILE_SCOPE("app_game_loop");

		// Application loop
		while (m_IsRunning) 
		{
			Timer timer;

			if (!m_WindowMinimized) 
			{
			{
				PROFILE_SCOPE("app_layers_update");
				for (AppLayer* layer : m_AppLayers)
				{
					layer->OnUpdate(m_FrameTime * m_TimeScale);
				}
			}

			#ifdef PT_EDITOR
			{
				PROFILE_SCOPE("imgui_render");
				EditorLayer* editorLayer = EditorLayer::Get();

				editorLayer->BeginImGuiRender();
				for (AppLayer* layer : m_AppLayers)
				{
					layer->OnImGuiRender();
				}
				editorLayer->EndImGuiRender();
			}
			#else // PROTON_DISTRIBUTION
				m_GameInstance->OnUpdate(m_FrameTime * m_TimeScale);
			#endif
			}

			// Update window
			m_Window->OnUpdate();
			m_FrameTime = timer.Elapsed();
		}

		PROFILE_END_SESSION();
	}

	GameInstance* Application::GetGameInstance()
	{
#ifdef PT_EDITOR
		return EditorLayer::GetMainGameInstance();
#else // PROTON_DISTRIBUTION
		return s_Instance->m_GameInstance.get();
#endif
	}

	void Application::PushLayer(AppLayer* layer)
	{
		m_AppLayers.emplace_back(layer);
		layer->OnCreate();
	}

	void Application::PushOverlay(AppLayer* layer)
	{
		m_AppLayers.insert(m_AppLayers.begin(), layer);
		layer->OnCreate();
	}

	void Application::Exit()
	{
		m_IsRunning = false;
	}

	static Timer s_AppTimer;

	float Application::GetTotalTimeElapsed()
	{
		return s_AppTimer.Elapsed();
	}	

	void Application::OnEvent(Event& event)
	{
		PROFILE_FUNCTION();
		EventDispatcher dispatcher(event);
		
		dispatcher.Dispatch<KeyPressedEvent>([&](KeyPressedEvent& e)
		{
			if (e.GetKeyCode() == Key::F11)
				m_Window->SetFullscreen(!m_Window->IsFullscreen());
			return false;
		});

		dispatcher.Dispatch<WindowCloseEvent>([&](WindowCloseEvent& e)
		{
			m_IsRunning = false;
			return true;
		});

	#ifdef PROTON_DISTRIBUTION
		dispatcher.Dispatch<WindowResizeEvent>([&](WindowResizeEvent& e)
		{
			uint32_t width = e.GetWidth(), height = e.GetHeight();
			if (width && height)
			{
				m_WindowMinimized = false;
				Renderer::SetViewport(0, 0, width, height);

				for (auto& kv : m_GameInstance->GetSceneManager()->m_Scenes)
					kv.second->OnViewportResize(width, height);
			}
			else 
				m_WindowMinimized = true;

			return false;
		});
	#endif

		for (AppLayer* layer : m_AppLayers)
		{
			if (event.Handled)
				return;
			layer->OnEvent(event);
		}

	#ifdef PROTON_DISTRIBUTION
		if (!event.Handled)
		{
			if (Scene* scene = m_GameInstance->GetActiveScene())
			{
				if (scene->IsSimulated())
					scene->GetGameMode()->OnEvent(event);
			}
		}
	#endif
	}

}
