#pragma once

#include "Proton/Core/Base.h"
#include "Proton/Core/AppConfig.h"
#include "Proton/Core/LayerStack.h"
#include "Proton/Core/Window.h"

namespace proton {

	// Forward declaration
	class GameInstance;

	class Application
	{
	public:
		Application();
		virtual ~Application();

		static Application& Get() { return *s_Instance; }

		void Run();
		void Exit();

		float GetTimeScale() const { return m_TimeScale; };
		float GetLastFrameTime() { return s_Instance->m_FrameTime; }
		float GetTotalTimeElapsed();

		Window& GetWindow() { return *m_Window; }
		const Window& GetWindow() const { return *m_Window; }
		static Shared<GameInstance> GetGameInstance();

	protected:
		virtual bool OnCreate() = 0; // To be defined by client

		void OnEvent(Event& event);

	private:
		static Application* s_Instance;

		Shared<GameInstance> m_GameInstance;
		Unique<Window> m_Window;

		AppConfig m_AppConfig;
		LayerStack m_LayerStack;

		bool m_IsRunning = false;
		bool m_WindowMinimized = false;
		float m_FrameTime = 0.0f;
		float m_TimeScale = 1.0f;

		friend class SettingsPanel;
		friend class InfoPanel;
	};

}

// Application Entry Point
#ifdef PROTON_DISTRIBUTION

	#ifdef PROTON_PLATFORM_WINDOWS

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
