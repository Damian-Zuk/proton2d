#include "pch.h"
#include "Application.h"
#include "Logger.h"
#include "Events/WindowEvents.h"

#include <iostream>
#include <glad/glad.h>

namespace proton {

#define BIND_EVENT_FN(x) std::bind(&x, this, std::placeholders::_1)

	Application::Application()
	{
		m_Window = Window::create();
		m_Window->setEventCallback(BIND_EVENT_FN(Application::onEvent));
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
				glClearColor(1, 0, 1, 1);
				glClear(GL_COLOR_BUFFER_BIT);
				m_Window->onUpdate();
			}
		}
	}

	void Application::onEvent(Event& event)
	{
		EventDispatcher dispatcher(event);

		dispatcher.dispatch<WindowClosedEvent>([&](WindowClosedEvent& e) -> bool {
			LOG_INFO("Window closed event occured");
			m_IsRunning = false;
			return true;
		});
	}

}