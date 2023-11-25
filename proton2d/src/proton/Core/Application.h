#pragma once

#include "proton/Core/Base.h"
#include "proton/Core/Window.h"
#include "proton/Core/AppLayer.h"
#include "proton/Editor/EditorLayer.h"

namespace proton {

	class Application
	{
	public:
		Application();
		virtual ~Application();

		void Run();
		void PushLayer(AppLayer* layer);
		void PushOverlay(AppLayer* layer);

		static Application& Get() { return *s_Instance; }
		Window& GetWindow() { return *m_Window; }

	protected:
		virtual bool OnCreate() = 0; // To be defined by client

		void OnEvent(Event& event);

	private:
		static Application* s_Instance;

		std::vector<AppLayer*> m_AppLayers;
		Unique<Window> m_Window;

		std::string m_Name;
		bool m_IsRunning = false;
		bool m_WindowMinimized = false;
		float m_FrameTime = 0.0f;
		float m_TimeScale = 1.0f;

		EditorLayer* m_EditorLayer;
		bool m_ShowEditorOverlay = true;

		friend class MiscellaneousPanel;
	};

}

// Application Entry Point
#ifdef PROTON_DISTRIBUTION
	#ifdef PROTON_PLATFORM_WINDOWS
		#include <Windows.h>
		#define PROTON_APPLICATION_ENTRY_POINT(ApplcationClass)\
		int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hInstPrev, PSTR cmdline, int cmdshow)\
		{\
			proton::Logger::Init();\
			ApplcationClass app;\
			app.Run();\
		}
	#endif
#else
#define PROTON_APPLICATION_ENTRY_POINT(ApplcationClass)\
	int main(int argc, char** argv)\
	{\
		proton::Logger::Init();\
		ApplcationClass app;\
		app.Run();\
	}
#endif
