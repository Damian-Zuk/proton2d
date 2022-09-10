#pragma once
#include "proton/Core/Core.h"
#include "proton/Core/Window.h"
#include "proton/Core/Layer.h"
#include "proton/Core/LayerStack.h"
#include "proton/ImGui/ImGuiLayer.h"

namespace proton {

	class Application
	{
	public:
		Application();
		~Application();
		void Run();
		
		void PushLayer(Layer* layer);
		void PushOverlay(Layer* layer);

		static inline Application& Get() { return *s_Instance; }
		inline Window& GetWindow() { return *m_Window; }
	protected:
		virtual bool OnCreate() = 0;
		void OnEvent(Event& event);

	private:
		static Application* s_Instance;
		bool m_IsRunning = true;
		bool m_WindowMinimized = false;
		float m_LastFrameTime = 0.0f;
		Unique<Window> m_Window;
		LayerStack m_LayerStack;
		ImGuiLayer* m_ImGuiLayer;
	};

	Application* CreateApp(); // entry point (main function)
}
