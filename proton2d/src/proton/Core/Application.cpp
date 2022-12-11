#include "pch.h"
#include "proton/Core/Application.h"
#include "proton/Core/Input.h"
#include "proton/Events/WindowEvents.h"
#include "proton/Graphics/Renderer.h"
#include "proton/Entity/ScriptFactory.h"

#ifdef PROTON_PLATFORM_WINDOWS
	#include "proton/Platform/Windows/WindowsWindow.h"
	#include "proton/Platform/Windows/WindowsInput.h"
#endif

#include <chrono>

namespace proton {

	Application* Application::s_Instance = nullptr;
	Input* Input::s_Instance = nullptr;


	Application::Application(const std::string& appName)
		: m_AppName(appName)
	{
		Application::s_Instance = this;

		#ifdef PROTON_PLATFORM_WINDOWS
			m_Window = CreateUnique<WindowsWindow>(appName, 1600, 900);
			Input::s_Instance = new WindowsInput();
		#endif

		m_Window->SetEventCallback(BIND_FUNCTION(Application::OnEvent));

		#ifndef PROTON_DISTRIBUTION
			m_EditorOverlay = new EditorOverlay();
			PushOverlay(m_EditorOverlay);
		#endif
	}

	Application::~Application()
	{
		for (AppLayer* layer : m_AppLayers)
		{
			layer->OnDestroy();
			delete layer;
		}
	}

	void Application::Run()
	{
		PROFILE_BEGIN_SESSION("Proton-Runtime");

		Logger::init();
		Renderer::Init();
		ScriptFactory::Init();

		if (OnCreate()) 
		{
			while (m_IsRunning) 
			{
				auto start = std::chrono::high_resolution_clock::now();

				if (!m_WindowMinimized) 
				{
					PROFILE_SCOPE("AppLayers OnUpdate");
					for (AppLayer* layer : m_AppLayers)
						layer->OnUpdate(m_LastFrameTime);
				}

				#ifndef PROTON_DISTRIBUTION
				{
					PROFILE_SCOPE("ImGuiRender");
					m_EditorOverlay->m_DebugInfo.m_FrameTime = m_LastFrameTime;
					m_EditorOverlay->BeginImGuiRender();

					for (AppLayer* layer : m_AppLayers)
						layer->OnImGuiRender();

					m_EditorOverlay->EndImGuiRender();
				}
				#endif

				m_Window->OnUpdate();
				
				auto end = std::chrono::high_resolution_clock::now();
				m_LastFrameTime = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count() / 1000000.0f;
			}
		}

		PROFILE_END_SESSION();
	}

	void Application::PushLayer(AppLayer* layer)
	{
		m_AppLayers.insert(m_AppLayers.begin(), layer);
		layer->OnCreate();
	}

	void Application::PushOverlay(AppLayer* layer)
	{
		m_AppLayers.emplace_back(layer);
		layer->OnCreate();
	}

	void Application::OnEvent(Event& event)
	{
		EventDispatcher dispatcher(event);

		dispatcher.Dispatch<WindowClosedEvent>([&](WindowClosedEvent& e) -> bool {
			
			m_IsRunning = false;
			return true;
		});

		dispatcher.Dispatch<WindowResizedEvent>([&](WindowResizedEvent& e) -> bool {
			
			uint32_t width = e.GetWidth(), height = e.GetHeight();
			if (width && height)
			{
				m_WindowMinimized = false;
				Renderer::SetViewport(0, 0, width, height);
			}
			else 
				m_WindowMinimized = true;

			return false;
		});

		for (AppLayer* layer : m_AppLayers)
			layer->OnEvent(event);
	}

}