#pragma once
#include "Core/Core.h"
#include "Core/Window.h"
#include "Core/Layer.h"
#include "Core/LayerStack.h"
#include "ImGui/ImGuiLayer.h"

namespace proton {

	class Application
	{
	public:
		Application();
		~Application();
		void run();
		
		void pushLayer(Layer* layer);
		void pushOverlay(Layer* layer);

		static inline Application& get() { return *s_Instance; }
		inline Window& getWindow() { return *m_Window; }
	protected:
		virtual bool onCreate() = 0;
		void onEvent(Event& event);

	private:
		static Application* s_Instance;
		bool m_IsRunning = true;
		float m_LastFrameTime = 0.0f;
		Unique<Window> m_Window;
		LayerStack m_LayerStack;
		ImGuiLayer* m_ImGuiLayer;
	};

	Application* createApp(); // entry point (main function)
}
