#include "pch.h"
#include "proton/Core/Application.h"
#include "proton/Core/Input.h"
#include "proton/Events/WindowEvents.h"
#include "proton/Events/KeyEvents.h"
#include "proton/Graphics/Renderer.h"
#include "proton/Scene/ScriptFactory.h"
#include "proton/Scene/SceneManager.h"
#include "proton/Core/Utils.h"

#ifdef PROTON_PLATFORM_WINDOWS
	#include "proton/Platform/Windows/WindowsWindow.h"
	#include "proton/Platform/Windows/WindowsInput.h"
#endif

#if PROTON_EDITOR
	#include <imgui.h>
#endif

#include <chrono>
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace proton {

	Application* Application::s_Instance = nullptr;
	Input* Input::s_Instance = nullptr;
	static glm::vec4 s_ScreenClearColor = { 0.1f, 0.12f, 0.16f, 1.0f };

	Application::Application(const std::string& appName)
		: m_AppName(appName)
	{
		Application::s_Instance = this;

		// Default values
		uint32_t windowWidth = 1600;
		uint32_t windowHeight = 900;
		bool VSync = false;
		bool Fullscreen = false;

		if (std::filesystem::exists("app.config.json"))
		{
			// Load config file
			std::string configData = ReadFileBinary("app.config.json");
			json jsonObj = jsonObj.parse(configData);
			windowWidth = jsonObj["Window_Width"];
			windowHeight = jsonObj["Window_Height"];
			VSync = jsonObj["VSync"];
			Fullscreen = jsonObj["Fullscreen"];
		}	
		else
		{
			// Create config file if it doesn't exists
			json jsonObj;
			jsonObj["Window_Width"] = windowWidth;
			jsonObj["Window_Height"] = windowHeight;
			jsonObj["VSync"] = VSync;
			jsonObj["Fullscreen"] = Fullscreen;
			std::ofstream configFile("app.config.json");
			configFile << jsonObj.dump(4);
			configFile.close();
		}

#ifdef PROTON_PLATFORM_WINDOWS
		m_Window = CreateUnique<WindowsWindow>
			(m_AppName, windowWidth, windowHeight);
		Input::s_Instance = new WindowsInput();
#endif

		m_Window->SetEventCallback(BIND_FUNCTION(Application::OnEvent));
		m_Window->SetVSync(VSync);
		if (Fullscreen)
			m_Window->SetFullscreen(true);

#if PROTON_EDITOR
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

		delete Input::s_Instance;
		delete SceneManager::s_Instance;
	}

	void Application::Run()
	{
		if (m_IsRunning)
			return;

		PROFILE_BEGIN_SESSION("Proton-Runtime");

		Logger::Init();
		Renderer::Init();
		Renderer::SetClearColor(s_ScreenClearColor);
		SceneManager::s_Instance = new SceneManager();

		if (OnCreate()) 
		{
			m_IsRunning = true;
			while (m_IsRunning) 
			{
				auto start = std::chrono::high_resolution_clock::now();

				if (!m_WindowMinimized) 
				{
					{
						PROFILE_SCOPE("app_layers_on_update");
						for (AppLayer* layer : m_AppLayers)
							layer->OnUpdate(m_FrameTime);
					}
					
					Renderer::Clear();
					Scene* activeScene = SceneManager::GetActiveScene();
					if (activeScene)
						activeScene->OnUpdate(m_FrameTime);
				}

				#if PROTON_EDITOR
				if (m_ShowEditorOverlay)
				{
					PROFILE_SCOPE("imgui_render")
					m_EditorOverlay->m_DebugInfo.m_FrameTime = m_FrameTime;
					m_EditorOverlay->BeginImGuiRender();

					for (AppLayer* layer : m_AppLayers)
						layer->OnImGuiRender();

					m_EditorOverlay->EndImGuiRender();
				}
				#endif

				m_Window->OnUpdate();
				
				auto end = std::chrono::high_resolution_clock::now();
				m_FrameTime = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count() / 1000000.0f;
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

		dispatcher.Dispatch<KeyPressedEvent>([&](KeyPressedEvent& e) -> bool {
#if PROTON_EDITOR
			if (e.GetKeyCode() == Key::F3)
				m_ShowEditorOverlay = !m_ShowEditorOverlay;
#endif
			return false;
		});

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