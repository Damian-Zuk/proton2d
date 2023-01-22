/*
* From: https://github.com/TheCherno/Hazel/blob/master/Hazel/src/Platform/Windows/WindowsWindow.cpp
*/

#include "pch.h"
#include "Proton/Platform/Windows/WindowsWindow.h"
#include "Proton/Events/WindowEvents.h"
#include "Proton/Events/KeyEvents.h"
#include "Proton/Events/MouseEvents.h"

#include "glad/glad.h"

namespace proton {

	static uint8_t s_GLFWWindowCount = 0;

	static void GLFWErrorCallback(int error, const char* description)
	{
		LOG_ERROR("GLFW Error", error, ':', description);
	}

	WindowsWindow::WindowsWindow(const std::string& title, uint32_t width, uint32_t height)
	{
		m_Data.Title = title;
		m_Data.Width = width;
		m_Data.Height = height;
		m_Data.Fullscreen = false;
		m_PreviousWidth = width;
		m_PreviousHeight = height;

		LOG_INFO("Creating window", title, "(", width, height, ")");

		if (s_GLFWWindowCount == 0)
		{
			int success = glfwInit();
			glfwSetErrorCallback(GLFWErrorCallback);
		}

		{
			m_Window = glfwCreateWindow((int)width, (int)height, m_Data.Title.c_str(), nullptr, nullptr);
			++s_GLFWWindowCount;
		}

		// Context
		glfwMakeContextCurrent(m_Window);
		gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
		glfwSetWindowUserPointer(m_Window, &m_Data);
		SetVSync(true);

		LOG_INFO("OpenGL version:", glGetString(GL_VERSION));
		LOG_INFO("Renderer:", glGetString(GL_RENDERER));

		// GLFW event callbacks
		glfwSetWindowSizeCallback(m_Window, [](GLFWwindow* window, int width, int height)
			{
				WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);
				data.Width = width;
				data.Height = height;

				WindowResizedEvent event(width, height);
				data.EventCallback(event);
			});

		glfwSetWindowCloseCallback(m_Window, [](GLFWwindow* window)
			{
				WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);
				WindowClosedEvent event;
				data.EventCallback(event);
			});

		glfwSetKeyCallback(m_Window, [](GLFWwindow* window, int key, int scancode, int action, int mods)
			{
				WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

				switch (action)
				{
				case GLFW_PRESS:
				{
					KeyPressedEvent event(key, 0);
					data.EventCallback(event);
					break;
				}
				case GLFW_RELEASE:
				{
					KeyReleasedEvent event(key);
					data.EventCallback(event);
					break;
				}
				case GLFW_REPEAT:
				{
					KeyPressedEvent event(key, 1);
					data.EventCallback(event);
					break;
				}
				}
			});

		glfwSetCharCallback(m_Window, [](GLFWwindow* window, unsigned int keycode)
			{
				WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

				KeyTypedEvent event(keycode);
				data.EventCallback(event);
			});

		glfwSetMouseButtonCallback(m_Window, [](GLFWwindow* window, int button, int action, int mods)
			{
				WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

				switch (action)
				{
				case GLFW_PRESS:
				{
					MouseButtonPressedEvent event(button);
					data.EventCallback(event);
					break;
				}
				case GLFW_RELEASE:
				{
					MouseButtonReleasedEvent event(button);
					data.EventCallback(event);
					break;
				}
				}
			});

		glfwSetScrollCallback(m_Window, [](GLFWwindow* window, double xOffset, double yOffset)
			{
				WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

				MouseScrolledEvent event((float)xOffset, (float)yOffset);
				data.EventCallback(event);
			});

		glfwSetCursorPosCallback(m_Window, [](GLFWwindow* window, double xPos, double yPos)
			{
				WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

				MouseMovedEvent event((float)xPos, (float)yPos);
				data.EventCallback(event);
			});
	}

	WindowsWindow::~WindowsWindow()
	{
		Shutdown();
	}

	void WindowsWindow::Shutdown()
	{
		glfwDestroyWindow(m_Window);
		--s_GLFWWindowCount;

		if (s_GLFWWindowCount == 0)
			glfwTerminate();
		
		LOG_INFO("GLFW window destroyed")
	}

	void WindowsWindow::OnUpdate()
	{
		PROFILE_FUNCTION();
		glfwPollEvents();
		glfwSwapBuffers(m_Window);
	}

	void WindowsWindow::SetVSync(bool enabled)
	{
		if (enabled)
			glfwSwapInterval(1);
		else
			glfwSwapInterval(0);

		m_Data.VSync = enabled;
	}

	bool WindowsWindow::IsVSync() const
	{
		return m_Data.VSync;
	}

	void WindowsWindow::SetFullscreen(bool fullscreen)
	{
		m_Data.Fullscreen = fullscreen;
		GLFWmonitor* monitor = glfwGetPrimaryMonitor();
		if (fullscreen)
		{
			m_PreviousWidth = m_Data.Width;
			m_PreviousHeight = m_Data.Height;
			const GLFWvidmode* mode = glfwGetVideoMode(monitor);
			glfwSetWindowMonitor(m_Window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
		}
		else
		{
			m_Data.Width = m_PreviousWidth;
			m_Data.Height = m_PreviousWidth;
			glfwSetWindowMonitor(m_Window, NULL, 100, 100, m_Data.Width, m_Data.Height, GLFW_DONT_CARE);
		}
	}

	bool WindowsWindow::IsFullscreen() const
	{
		return m_Data.Fullscreen;
	}

}