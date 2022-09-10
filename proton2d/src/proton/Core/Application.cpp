#include "pch.h"
#include "proton/Core/Application.h"
#include "proton/Core/Logger.h"
#include "proton/Events/WindowEvents.h"
#include "proton/Renderer/Renderer.h"

#include <iostream>
#include <GLFW/glfw3.h>

namespace proton {

	Application* Application::s_Instance = nullptr;

	Application::Application()
		: m_ImGuiLayer(new ImGuiLayer())
	{
		s_Instance = this;
		m_Window = Window::Create();
		m_Window->SetEventCallback(BIND_FUNCTION(Application::OnEvent));
		PushOverlay(m_ImGuiLayer);
	}

	Application::~Application()
	{
	}

	void Application::Run()
	{
		Logger::init();
		Renderer::Init();

		if (OnCreate()) 
		{
			while (m_IsRunning) 
			{
				float time = (float)glfwGetTime();
				float timestep = time - m_LastFrameTime;
				m_LastFrameTime = time;

				if (!m_WindowMinimized) 
				{
					for (Layer* layer : m_LayerStack)
						layer->OnUpdate(timestep);
				}

				m_ImGuiLayer->Begin();
				for (Layer* layer : m_LayerStack)
					layer->OnImGuiRender();
				m_ImGuiLayer->End();

				m_Window->OnUpdate();
			}
		}
	}

	void Application::PushLayer(Layer* layer)
	{
		m_LayerStack.PushLayer(layer);
		layer->OnAttach();
	}

	void Application::PushOverlay(Layer* layer)
	{
		m_LayerStack.PushOverlay(layer);
		layer->OnAttach();
	}

	void Application::OnEvent(Event& event)
	{
		EventDispatcher dispatcher(event);

		dispatcher.Dispatch<WindowClosedEvent>([&](WindowClosedEvent& e) -> bool {

			m_IsRunning = false;

			return true;
		});

		for (Layer* layer : m_LayerStack)
			layer->OnEvent(event);
	}

}