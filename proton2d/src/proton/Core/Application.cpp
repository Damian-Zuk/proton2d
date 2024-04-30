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
		NetworkManager::StaticInit();

	#ifdef PT_EDITOR
		m_EditorLayer = EditorLayer::Get();
		PushOverlay(m_EditorLayer);
	#endif

		PrefabManager::Init();
		m_GameInstance->Init();

		PROFILE_BEGIN_SESSION("Proton-Runtime");

		if (OnCreate()) 
		{
			m_IsRunning = true;

			// The game loop
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
						// Update application layers
						PROFILE_SCOPE("app_layers_on_update");
						for (AppLayer* layer : m_AppLayers)
							layer->OnUpdate(m_FrameTime * m_TimeScale);
					}

				#ifdef PT_EDITOR
					// Update and render Editor ImGui interface
					{
						PROFILE_SCOPE("imgui_render");
					
						m_EditorLayer->BeginImGuiRender();

						for (AppLayer* layer : m_AppLayers)
							layer->OnImGuiRender();

						m_EditorLayer->EndImGuiRender();
					}
				#else
					m_GameInstance->OnUpdate(m_FrameTime * m_TimeScale);
				#endif
				}

				// Update window
				m_Window->OnUpdate();
				
				m_FrameTime = timer.Elapsed();
			}
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
			layer->OnEvent(event);
		}

		Scene* scene = m_GameInstance->GetActiveScene();
		if (scene)
			scene->GetGameMode()->OnEvent(event);
	}

}
