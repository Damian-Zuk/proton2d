#include "pch.h"
#include "ImGuiLayer.h"
#include "proton/Application.h"
#include "proton/Core/Window.h"
#include "proton/Platform/OpenGL/imgui_impl_opengl3.h"


namespace proton {
	ImGuiLayer::ImGuiLayer()
		: Layer("ImGui Layer")
	{
	}

	ImGuiLayer::~ImGuiLayer()
	{
	}

	void ImGuiLayer::onAttach()
	{
		ImGui::CreateContext();
		ImGui::StyleColorsDark();

		ImGuiIO& io = ImGui::GetIO();
		io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;
		io.BackendFlags |= ImGuiBackendFlags_HasSetMousePos;
		ImGui_ImplOpenGL3_Init("#version 410");
	}

	void ImGuiLayer::onDetach()
	{
	}

	void ImGuiLayer::onUpdate(float timestep)
	{
		ImGuiIO& io = ImGui::GetIO();
		Window& win = Application::get().getWindow();
		io.DisplaySize = ImVec2(win.getWidth(), win.getHeight());

		ImGui_ImplOpenGL3_NewFrame();
		ImGui::NewFrame();
		static bool show = true;
		ImGui::ShowDemoWindow(&show);
		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
	}

	void ImGuiLayer::onEvent(Event& event)
	{
		EventDispatcher dispather(event);
		dispather.dispatch<MouseMovedEvent>(BIND_EVENT_FN(ImGuiLayer::onMouseMoveEvent));
		dispather.dispatch<MouseButtonPressedEvent>(BIND_EVENT_FN(ImGuiLayer::onMouseButtonPressedEvent));
		dispather.dispatch<MouseButtonReleasedEvent>(BIND_EVENT_FN(ImGuiLayer::onMouseButtonReleasedEvent));
		dispather.dispatch<MouseScrolledEvent>(BIND_EVENT_FN(ImGuiLayer::onMouseScrolledEvent));
	}

	bool ImGuiLayer::onMouseMoveEvent(MouseMovedEvent& e)
	{
		ImGuiIO& io = ImGui::GetIO();
		io.MousePos = ImVec2(e.getX(), e.getY());
		return false;
	}

	bool ImGuiLayer::onMouseButtonPressedEvent(MouseButtonPressedEvent& e)
	{
		ImGuiIO& io = ImGui::GetIO();
		io.MouseDown[e.GetMouseButton()] = true;
		return false;
	}

	bool ImGuiLayer::onMouseButtonReleasedEvent(MouseButtonReleasedEvent& e)
	{
		ImGuiIO& io = ImGui::GetIO();
		io.MouseDown[e.GetMouseButton()] = false;
		return false;
	}

	bool ImGuiLayer::onMouseScrolledEvent(MouseScrolledEvent& e)
	{
		ImGuiIO& io = ImGui::GetIO();
		io.MouseWheelH += e.getXOffset();
		io.MouseWheel += e.getYOffset();
		return false;
	}

}
