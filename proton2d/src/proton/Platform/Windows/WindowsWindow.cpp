#include "ptpch.h"
#include "Proton/Platform/Windows/WindowsWindow.h"
#include "Proton/Events/WindowEvents.h"
#include "Proton/Events/KeyEvents.h"
#include "Proton/Events/MouseEvents.h"

#include <glad/glad.h>
#include <stb_image.h>

namespace proton {

	static uint8_t s_GLFWWindowCount = 0;

	static void GLFWErrorCallback(int error, const char* description)
	{
		PT_CORE_ERROR("GLFW Error {}: {}", error, description);
	}

	WindowsWindow::WindowsWindow(const WindowSpecification& spec)
		: Window(spec), m_PreviousWidth(spec.Width), m_PreviousHeight(spec.Height)
	{
		_PT_CORE_INFO("*********************************************************");
		_PT_CORE_INFO("Creating window '{}' ({}x{})", spec.Title, spec.Width, spec.Height);

		if (s_GLFWWindowCount == 0)
		{
			int success = glfwInit();
			glfwSetErrorCallback(GLFWErrorCallback);
		}

		m_Window = glfwCreateWindow((int)spec.Width, (int)spec.Height, spec.Title.c_str(), nullptr, nullptr);
		++s_GLFWWindowCount;

		// Context
		glfwMakeContextCurrent(m_Window);
		gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
		glfwSetWindowUserPointer(m_Window, &m_Data);
		SetVSync(true);

		_PT_CORE_INFO("OpenGL version: {}", reinterpret_cast<const char*>(glGetString(GL_VERSION)));
		_PT_CORE_INFO("Renderer: {}", reinterpret_cast<const char*>(glGetString(GL_RENDERER)));
		_PT_CORE_INFO("*********************************************************");

		// Set window icon
		int icon_width, icon_height, icon_channels;
		if (unsigned char* iconImage = stbi_load("../resources/icon.png", &icon_width, &icon_height, &icon_channels, 0))
		{
			GLFWimage icon;
			icon.width = icon_width;
			icon.height = icon_height;
			icon.pixels = iconImage;
			glfwSetWindowIcon(m_Window, 1, &icon);
		}

		using WindowData = Window::WindowData;

		// GLFW event callbacks
		glfwSetWindowSizeCallback(m_Window, [](GLFWwindow* window, int width, int height)
			{
				WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);
				data.Spec.Width = width;
				data.Spec.Height = height;

				WindowResizeEvent event(width, height);
				data.EventCallback(event);
			});

		glfwSetWindowCloseCallback(m_Window, [](GLFWwindow* window)
			{
				WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);
				WindowCloseEvent event;
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
		
		PT_CORE_INFO("GLFW Window terminated.");
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

		GetWindowSpecification().VSync = enabled;
	}

	void WindowsWindow::SetFullscreen(bool fullscreen)
	{
		auto& spec = GetWindowSpecification();

		if (fullscreen != spec.Fullscreen)
		{
			spec.Fullscreen = fullscreen;
			GLFWmonitor* monitor = glfwGetPrimaryMonitor();
			if (fullscreen)
			{
				m_PreviousWidth = spec.Width;
				m_PreviousHeight = spec.Height;
				const GLFWvidmode* mode = glfwGetVideoMode(monitor);
				glfwSetWindowMonitor(m_Window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
			}
			else
			{
				spec.Width = m_PreviousWidth;
				spec.Height = m_PreviousWidth;
				glfwSetWindowMonitor(m_Window, NULL, 100, 100, spec.Width, spec.Height, GLFW_DONT_CARE);
			}
		}
	}

}
