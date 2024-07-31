#include "ptpch.h"
#include "Proton/Core/Application.h"
#include "Proton/Core/Timer.h"
#include "Proton/Core/Input.h"
#include "Proton/Core/GameInstance.h"

#include "Proton/Events/WindowEvents.h" 
#include "Proton/Events/KeyEvents.h"
#include "Proton/Events/MouseEvents.h"

#include "Proton/Graphics/Renderer/Renderer.h"
#include "Proton/Assets/AssetManager.h"
#include "Proton/Scene/SceneManager.h"
#include "Proton/Scene/PrefabManager.h"
#include "Proton/Scripting/GameModeBase.h"

#include "Proton/Network/Common/NetworkManager.h"

#ifdef PROTON_PLATFORM_WINDOWS
	#include "Proton/Platform/Windows/WindowsWindow.h"
#endif

#ifdef PT_EDITOR
	#include "Proton/Editor/EditorLayer.h"
#endif

namespace proton {

	Application* Application::s_Instance = nullptr;

	Application::Application()
	{
		PT_CORE_ASSERT(!s_Instance, "Application already exists!");
		Application::s_Instance = this;

	#ifdef PROTON_PLATFORM_WINDOWS
		m_Window = MakeUnique<WindowsWindow>(m_AppConfig.WindowTitle, m_AppConfig.WindowWidth, m_AppConfig.WindowHeight);
	#endif

		m_Window->SetEventCallback(PT_BIND_FUNCTION(Application::OnEvent));
		m_Window->SetFullscreen(m_AppConfig.Fullscreen);
		m_Window->SetVSync(m_AppConfig.VSync);

		m_GameInstance = MakeUnique<GameInstance>();
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
		EditorLayer::s_Instance = new EditorLayer(m_GameInstance.get());
		PushOverlay(EditorLayer::s_Instance);
	#endif

		if (!OnCreate())
		{
			PT_CORE_ERROR("Application initialization failed! Exiting.");
			return;
		}

		m_IsRunning = true;
		m_GameInstance->Init();

		Renderer::SetViewport(0, 0, m_Window->GetWidth(), m_Window->GetHeight());

		PROFILE_BEGIN_SESSION("Proton-Runtime");

		// Application loop
		while (m_IsRunning) 
		{
			PROFILE_SCOPE("app_game_loop");
			Timer timer;

			if (!m_WindowMinimized) 
			{
			#ifndef PT_EDITOR
				Renderer::Clear();
			#endif
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
			#else // Distribution
				m_GameInstance->OnUpdate(m_FrameTime * m_TimeScale);
			#endif
			}

			// Update window
			m_Window->OnUpdate();
			m_FrameTime = timer.Elapsed();
		}

		PROFILE_END_SESSION();
	}

	void Application::PushLayer(AppLayer* layer)
	{
		m_AppLayers.emplace_back(layer);
		layer->m_GameInstance = m_GameInstance.get();
		layer->OnCreate();
	}

	void Application::PushOverlay(AppLayer* layer)
	{
		m_AppLayers.insert(m_AppLayers.begin(), layer);
		layer->m_GameInstance = m_GameInstance.get();
		layer->OnCreate();
	}

	void Application::Exit()
	{
		m_IsRunning = false;
	}

	void Application::OnEvent(Event& event)
	{
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

	#ifndef PT_EDITOR
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
				break;
			layer->OnEvent(event);
		}

		if (!event.Handled)
		{
			if (Scene* scene = m_GameInstance->GetActiveScene())
				scene->GetGameMode()->OnEvent(event);
		}
	}

}
