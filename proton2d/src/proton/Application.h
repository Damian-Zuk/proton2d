#pragma once
#include "Core/Core.h"
#include "Core/Window.h"

namespace proton {

	class PROTON_API Application
	{
	public:
		Application();
		~Application();
		void run();
	
	protected:
		virtual bool onCreate() = 0;
		void onEvent(Event& event);

	private:
		bool m_IsRunning = true;
		Window* m_Window;
	};

	Application* createApp(); // entry point (main function)
}
