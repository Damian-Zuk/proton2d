#pragma once

#include "proton/Core/Base.h"
#include "proton/Core/Window.h"
#include "proton/Core/AppLayer.h"
#include "proton/Editor/EditorLayer.h"

namespace proton {

	class Application
	{
	public:
		Application(const std::string& appName);
		virtual ~Application();

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
		EditorLayer* m_EditorLayer;
		bool m_ShowEditorOverlay = true;

		friend class MiscellaneousPanel;
	};

}

#ifdef PROTON_DISTRIBUTION
#ifdef PROTON_PLATFORM_WINDOWS
#include <Windows.h>
#define PROTON_APPLICATION_ENTRY_POINT(ApplcationClass, WindowTitle)\
	int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hInstPrev, PSTR cmdline, int cmdshow)\
	{\
		ApplcationClass app(WindowTitle);\
		app.Run();\
	}
#endif
#else
#define PROTON_APPLICATION_ENTRY_POINT(ApplcationClass, WindowTitle)\
	int main(int argc, char** argv)\
	{\
		ApplcationClass app(WindowTitle);\
		app.Run();\
	}
#endif

	
