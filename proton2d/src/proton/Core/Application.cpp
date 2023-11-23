#include "pch.h"
#include "proton/Core/Application.h"
#include "proton/Core/Input.h"
#include "proton/Events/WindowEvents.h"
#include "proton/Events/KeyEvents.h"
#include "proton/Graphics/Renderer/Renderer.h"
#include "proton/Scene/ScriptFactory.h"
#include "proton/Assets/AssetManager.h"
#include "proton/Scene/SceneManager.h"
#include "proton/Scene/PrefabManager.h"
#include "proton/Utils/Utils.h"

#ifdef PROTON_PLATFORM_WINDOWS
	#include "proton/Platform/Windows/WindowsWindow.h"
	#include "proton/Platform/Windows/WindowsInput.h"
#endif

#ifdef PT_EDITOR
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

	Application::Application(const std::string& appName)
		: m_AppName(appName)
	{
		Application::s_Instance = this;

		// TODO: Move this to ApplicationProporties class
		// Default values
		uint32_t windowWidth = 1600;
		uint32_t windowHeight = 900;
		bool VSync = false;
		bool Fullscreen = false;

		if (std::filesystem::exists("app.config.json"))
		{
			// Load config file
			std::string configData = Utils::ReadFile("app.config.json");
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

		m_Window->SetEventCallback(PT_BIND_FUNCTION(Application::OnEvent));
		m_Window->SetVSync(VSync);
		if (Fullscreen)
			m_Window->SetFullscreen(true);

#ifdef PT_EDITOR
		m_EditorLayer = new EditorLayer();
		PushOverlay(m_EditorLayer);
#endif
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
			return;

		AssetManager::Init();
		SceneManager::Init();
		PrefabManager::Init();
		Renderer::Init();

		PROFILE_BEGIN_SESSION("Proton-Runtime");

		if (OnCreate()) 
		{
			m_IsRunning = true;
			while (m_IsRunning) 
			{
				PROFILE_SCOPE("app_loop");
				auto start = std::chrono::high_resolution_clock::now();

				if (!m_WindowMinimized) 
				{
					Renderer::Clear();
					{
						PROFILE_SCOPE("app_layers_on_update");
						for (AppLayer* layer : m_AppLayers)
							layer->OnUpdate(m_FrameTime * m_TimeScale);
					}

					Scene* activeScene = SceneManager::GetActiveScene();
					if (activeScene)
						activeScene->OnUpdate(m_FrameTime * m_TimeScale);
#ifdef PT_EDITOR
					if (m_ShowEditorOverlay)
					{
						PROFILE_SCOPE("imgui_render");
						m_EditorLayer->BeginImGuiRender();

						for (AppLayer* layer : m_AppLayers)
							layer->OnImGuiRender();

						m_EditorLayer->EndImGuiRender();
					}
#endif
				}

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

		
		dispatcher.Dispatch<KeyPressedEvent>([&](KeyPressedEvent& e)
		{
#ifdef PT_EDITOR
			if (e.GetKeyCode() == Key::F4)
				m_ShowEditorOverlay = !m_ShowEditorOverlay;
#endif
			if (e.GetKeyCode() == Key::F11)
				m_Window->SetFullscreen(!m_Window->IsFullscreen());

			return true;
		});
		

		dispatcher.Dispatch<WindowClosedEvent>([&](WindowClosedEvent& e)
		{
			m_IsRunning = false;
			return true;
		});

		dispatcher.Dispatch<WindowResizedEvent>([&](WindowResizedEvent& e)
		{
			uint32_t width = e.GetWidth(), height = e.GetHeight();
			if (width && height)
			{
				m_WindowMinimized = false;
				Renderer::SetViewport(0, 0, width, height);

				for (auto& kv : SceneManager::s_Instance->m_Scenes)
					kv.second->OnViewportResize(width, height);
			}
			else 
				m_WindowMinimized = true;

			return false;
		});

		for (AppLayer* layer : m_AppLayers)
			layer->OnEvent(event);
	}

}
