#include "pch.h"
#include "Proton/Core/Application.h"
#include "Proton/Core/Input.h"
#include "Proton/Events/WindowEvents.h" 
#include "Proton/Events/KeyEvents.h"
#include "Proton/Events/MouseEvents.h"
#include "Proton/Assets/AssetManager.h"
#include "Proton/Graphics/Renderer/Renderer.h"
#include "Proton/Scripting/ScriptFactory.h"
#include "Proton/Scene/SceneManager.h"
#include "Proton/Scene/PrefabManager.h"
#include "Proton/Utils/Utils.h"

#ifdef PROTON_PLATFORM_WINDOWS
	#include "Proton/Platform/Windows/WindowsWindow.h"
	#include "Proton/Platform/Windows/WindowsInput.h"
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

	Application::Application()
	{
		PT_CORE_ASSERT(!s_Instance, "Application already exists!");
		Application::s_Instance = this;

		// TODO: Move loading configuration to ApplicationProporties class
		uint32_t windowWidth = 1600;
		uint32_t windowHeight = 900;
		bool vsync = false;
		bool fullscreen = false;
		if (std::filesystem::exists("app.config.json"))
		{
			// Load config file
			std::string configData = Utils::ReadFile("app.config.json");
			json jsonObj = jsonObj.parse(configData);
			windowWidth = jsonObj["window_width"];
			windowHeight = jsonObj["window_height"];
			vsync = jsonObj["vsync"];
			fullscreen = jsonObj["fullscreen"];
			m_Name = jsonObj["application_name"];
		}	
		else
		{
			// Create config file if does not exists
			json jsonObj;
			jsonObj["window_width"] = windowWidth;
			jsonObj["window_height"] = windowHeight;
			jsonObj["vsync"] = vsync;
			jsonObj["fullscreen"] = fullscreen;
			jsonObj["application_name"] = m_Name;
			std::ofstream configFile("app.config.json");
			configFile << jsonObj.dump(4);
			configFile.close();
		}

	#ifdef PROTON_PLATFORM_WINDOWS
		m_Window = MakeUnique<WindowsWindow>(m_Name, windowWidth, windowHeight);
		Input::s_Instance = new WindowsInput();
	#endif

		m_Window->SetEventCallback(PT_BIND_FUNCTION(Application::OnEvent));
		m_Window->SetVSync(vsync);
		m_Window->SetFullscreen(fullscreen);

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
		{
			PT_CORE_ERROR("[Application::Run] Application is already running!");
			return;
		}

		AssetManager::Init();
		SceneManager::Init();
		PrefabManager::Init();
		Renderer::Init();

		PROFILE_BEGIN_SESSION("Proton-Runtime");

		if (OnCreate()) 
		{
			m_IsRunning = true;
			// Game loop
			while (m_IsRunning) 
			{
				PROFILE_SCOPE("app_game_loop");
				auto start = std::chrono::high_resolution_clock::now();

				if (!m_WindowMinimized) 
				{
					Renderer::Clear();
					{
						// Update application layers
						PROFILE_SCOPE("app_layers_on_update");
						for (AppLayer* layer : m_AppLayers)
							layer->OnUpdate(m_FrameTime * m_TimeScale);
					}

					// Update active scene
					// TODO: Can more than one scene be active?
					Scene* scene = SceneManager::GetActiveScene();
					if (scene)
						scene->OnUpdate(m_FrameTime * m_TimeScale);

				#ifdef PT_EDITOR
					// Update and render Editor ImGui interface
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

				// Update window
				m_Window->OnUpdate();
				
				// Calculate frame time
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
				if (ImGui::GetIO().WantCaptureKeyboard || ImGui::GetIO().WantTextInput)
					return true;

				if (e.GetKeyCode() == Key::F4)
					m_ShowEditorOverlay = !m_ShowEditorOverlay;
			#endif
				if (e.GetKeyCode() == Key::F11)
					m_Window->SetFullscreen(!m_Window->IsFullscreen());

				return false;
			});

		#ifdef PT_EDITOR
		dispatcher.Dispatch<MouseScrolledEvent>([&](MouseScrolledEvent& e)
			{
				if (ImGui::GetIO().WantCaptureMouse)
					return true;
				return false;
			});
		#endif

		#ifdef PT_EDITOR
		dispatcher.Dispatch<MouseButtonPressedEvent>([&](MouseButtonPressedEvent& e)
			{
				if (ImGui::GetIO().WantCaptureMouse)
					return true;
				return false;
			});
		#endif
		
		dispatcher.Dispatch<WindowCloseEvent>([&](WindowCloseEvent& e)
		{
			m_IsRunning = false;
			return true;
		});

		dispatcher.Dispatch<WindowResizeEvent>([&](WindowResizeEvent& e)
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
		{
			if (event.Handled)
				break;
			layer->OnEvent(event);
		}
	}

}
