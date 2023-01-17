#pragma once

#include "proton/Core/Core.h"
#include "proton/Core/Window.h"
#include "proton/Core/AppLayer.h"
#include "proton/Editor/EditorOverlay.h"

namespace proton {

	class Application
	{
	public:
		Application(const std::string& appName);
		virtual ~Application();

		// Call this function to run application
		// after you created Application derivative instance
		void Run();
		
		void PushLayer(AppLayer* layer);
		void PushOverlay(AppLayer* layer);

		void SetTimeScale(float timeScale);

		static Application& Get() { return *s_Instance; }
		Window& GetWindow() { return *m_Window; }

	protected:
		virtual bool OnCreate() = 0; // To be defined by client
		void OnEvent(Event& event);

	private:
		static Application* s_Instance;

		bool m_IsRunning = false;
		bool m_WindowMinimized = false;
		float m_FrameTime = 0.0f;
		float m_TimeScale = 1.0f;
		std::string m_AppName;
		
		std::vector<AppLayer*> m_AppLayers;
		Unique<Window> m_Window;
		EditorOverlay* m_EditorOverlay;
		bool m_ShowEditorOverlay = true;

		friend class DebugInfo;
	};

}
