#include <Proton2D.h>
#include <imgui/imgui.h>

using namespace proton;

class MainLayer : public proton::Layer {
public:

	MainLayer() : Layer("MainLayer") {}
	~MainLayer() {}

	virtual void onAttach() override {}
	virtual void onDetach() override {}

	virtual void onImGuiRender() override
	{
		static bool show = true;
		ImGui::ShowDemoWindow(&show);
	}
	
	virtual void onUpdate(float timestep) override 
	{
		if (Input::isKeyPressed(Key::F)) {
			LOG_INFO("F WAS PRESSED");
		}
	}
	
	void onEvent(proton::Event& event) {}
};

class Sandbox : public proton::Application
{
public:

	bool onCreate() override
	{
		LOG_OK("Hello from SandBox :)");
		pushLayer(new MainLayer());
		return true;
	}

};

proton::Application* proton::createApp() {
	return new Sandbox();
}