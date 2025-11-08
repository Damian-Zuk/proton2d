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
#include "Proton/Scripting/GameScript.h"

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

	Application::Application(const ApplicationSpecification& specification)
		: m_Specification(specification)
	{
		PT_CORE_ASSERT(!s_Instance, "Application already exists!");
		Application::s_Instance = this;

		if (!m_Specification.WorkingDirectory.empty())
			std::filesystem::current_path(m_Specification.WorkingDirectory);

		m_AppConfig.LoadConfig();

		WindowSpecification windowSpec;
		windowSpec.Title = m_Specification.Name;
		windowSpec.Width = m_AppConfig.WindowWidth;
		windowSpec.Height = m_AppConfig.WindowHeight;
		windowSpec.Fullscreen = m_AppConfig.Fullscreen;
		windowSpec.VSync = m_AppConfig.VSync;

	#ifdef PROTON_PLATFORM_WINDOWS
		m_Window = MakeUnique<WindowsWindow>(windowSpec);
	#endif
		m_Window->SetEventCallback(PT_BIND_FUNCTION(Application::OnEvent));

		AssetManager::Init();
		PrefabManager::Init();
		Renderer::Init();
	}

	void Application::Run()
	{
		if (m_IsRunning)
		{
			PT_CORE_ERROR("Application is already running!");
			return;
		}

		m_IsRunning = true;

	#ifdef PT_EDITOR
		EditorLayer* editorLayer = m_LayerStack.PushOverlay<EditorLayer>();
	#else
		m_GameInstance = MakeUnique<GameInstance>();
		m_GameInstance->Init();
		Renderer::SetViewport(0, 0, m_Window->GetWidth(), m_Window->GetHeight());
	#endif

		PROFILE_BEGIN_SESSION("Proton-Runtime");
		PROFILE_SCOPE("app_game_loop");

		// Main loop
		while (m_IsRunning) 
		{
			Timer timer;

			if (!m_WindowMinimized) 
			{
			{
				PROFILE_SCOPE("app_layers_update");
				for (Layer* layer : m_LayerStack)
				{
					layer->OnUpdate(m_FrameTime * m_TimeScale);
				}
			}

			#ifdef PT_EDITOR
			{
				PROFILE_SCOPE("imgui_render");
				editorLayer->BeginImGuiRender();
				for (Layer* layer : m_LayerStack)
				{
					layer->OnImGuiRender();
				}
				editorLayer->EndImGuiRender();
			}
			#else
				m_GameInstance->OnUpdate(m_FrameTime * m_TimeScale);
			#endif
			}

			m_Window->OnUpdate();
			m_FrameTime = timer.Elapsed();
		}

		Renderer::Shutdown();
		PROFILE_END_SESSION();
	}

	GameInstance* Application::GetGameInstance()
	{
#ifdef PT_EDITOR
		return EditorLayer::GetMainGameInstance();
#else
		return m_GameInstance.get();
#endif
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
			Exit();
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

		for (Layer* layer : m_LayerStack)
		{
			if (event.Handled)
				return;

			layer->OnEvent(event);
		}

		if (event.Handled)
			return;

	#ifdef PT_EDITOR
		Scene* scene = EditorLayer::GetFocusedGameInstance()->GetActiveScene();
	#else
		Scene* scene = m_GameInstance->GetActiveScene();
	#endif 

		if (scene && scene->IsSimulated())
			scene->GetGameScript()->OnEvent(event);
	}

}
