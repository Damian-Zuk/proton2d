#include "pch.h"
#include "Application.h"
#include "Logger.h"
#include "Events/WindowEvents.h"

#include <iostream>
#include <GLFW/glfw3.h>

namespace proton {

	Application* Application::s_Instance = nullptr;

	Application::Application()
		: m_ImGuiLayer(new ImGuiLayer())
	{
		s_Instance = this;
		m_Window = Window::create();
		m_Window->setEventCallback(BIND_EVENT_FN(Application::onEvent));
		pushOverlay(m_ImGuiLayer);
	}

	Application::~Application()
	{
		delete m_Window;
	}

	void Application::run()
	{
		Logger::init();
		LOG("Application started");

		// Game loop
		if (onCreate()) {
			while (m_IsRunning) {

				float time = (float)glfwGetTime();
				float timestep = time - m_LastFrameTime;
				m_LastFrameTime = time;

				for (Layer* layer : m_LayerStack)
					layer->onUpdate(timestep);

				m_ImGuiLayer->Begin();
				for (Layer* layer : m_LayerStack)
					layer->onImGuiRender();
				m_ImGuiLayer->End();

				m_Window->onUpdate();
			}
		}
	}

	void Application::pushLayer(Layer* layer)
	{
		m_LayerStack.pushLayer(layer);
		layer->onAttach();
	}

	void Application::pushOverlay(Layer* layer)
	{
		m_LayerStack.pushOverlay(layer);
		layer->onAttach();
	}

	void Application::onEvent(Event& event)
	{
		EventDispatcher dispatcher(event);

		dispatcher.dispatch<WindowClosedEvent>([&](WindowClosedEvent& e) -> bool {
			LOG_INFO("Window closed event occured");
			m_IsRunning = false;
			return true;
		});

		for (Layer* layer : m_LayerStack)
			layer->onEvent(event);
	}

}