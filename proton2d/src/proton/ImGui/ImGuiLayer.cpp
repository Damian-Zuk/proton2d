#include "pch.h"
#include "ImGuiLayer.h"
#include "proton/Application.h"
#include "proton/Core/Window.h"
#include "proton/Platform/OpenGL/imgui_impl_opengl3.h"

proton::ImGuiLayer::ImGuiLayer()
	: Layer("ImGui Layer")
{
}

proton::ImGuiLayer::~ImGuiLayer()
{
}

void proton::ImGuiLayer::onAttach()
{
	ImGui::CreateContext();
	ImGui::StyleColorsDark();

	ImGuiIO& io = ImGui::GetIO();
	io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;
	io.BackendFlags |= ImGuiBackendFlags_HasSetMousePos;
	ImGui_ImplOpenGL3_Init("#version 410");
}

void proton::ImGuiLayer::onDetach()
{
}

void proton::ImGuiLayer::onUpdate(float timestep)
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

void proton::ImGuiLayer::onEvent(Event& event)
{
}
