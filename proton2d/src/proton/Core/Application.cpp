#include "pch.h"
#include "proton/Core/Application.h"
#include "proton/Core/Input.h"
#include "proton/Events/WindowEvents.h"
#include "proton/Graphics/Renderer.h"
#include "proton/Entity/ScriptLoader.h"

#ifdef PROTON_PLATFORM_WINDOWS
	#include "proton/Platform/Windows/WindowsWindow.h"
	#include "proton/Platform/Windows/WindowsInput.h"
#endif

namespace proton {

	Application* Application::s_Instance = nullptr;
	Input* Input::s_Instance = nullptr;


	Application::Application(const std::string& appName)
		: m_AppName(appName), m_EditorOverlay(new EditorOverlay())
	{
		s_Instance = this;

	#ifdef PROTON_PLATFORM_WINDOWS
		m_Window = CreateUnique<WindowsWindow>(appName, 1600, 900);
		Input::s_Instance = new WindowsInput();
	#endif

		m_Window->SetEventCallback(BIND_FUNCTION(Application::OnEvent));
		PushOverlay(m_EditorOverlay);
	}

	Application::~Application()
	{
		for (Layer* layer : m_AppLayers)
		{
			layer->OnDetach();
			delete layer;
		}
	}

	void Application::Run()
	{
		Logger::init();
		Renderer::Init();
		ScriptLoader::Init();

		if (OnCreate()) 
		{
			while (m_IsRunning) 
			{
				float time = (float)glfwGetTime();
				float timestep = time - m_LastFrameTime;
				m_LastFrameTime = time;

				if (!m_WindowMinimized) 
				{
					for (Layer* layer : m_AppLayers)
						layer->OnUpdate(timestep);
				}

			#ifndef PROTON_DISTRIBUTION
				m_EditorOverlay->BeginImGuiRender();

				for (Layer* layer : m_AppLayers)
					layer->OnImGuiRender();

				m_EditorOverlay->EndImGuiRender();
			#endif

				m_Window->OnUpdate();
			}
		}
	}

	void Application::PushLayer(Layer* layer)
	{
		m_AppLayers.insert(m_AppLayers.begin(), layer);
		layer->OnAttach();
	}

	void Application::PushOverlay(Layer* layer)
	{
		m_AppLayers.emplace_back(layer);
		layer->OnAttach();
	}

	void Application::SetEditorActiveScene(const Shared<Scene>& scene)
	{
		m_EditorOverlay->SetActiveScene(scene);
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

		// Propagate event to layers
		for (Layer* layer : m_AppLayers)
			layer->OnEvent(event);
	}

}