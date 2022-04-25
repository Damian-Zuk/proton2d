#include <Proton2D.h>

class MainLayer : public proton::Layer {
public:

	MainLayer() : Layer("MainLayer") {}
	~MainLayer() {}

	void onAttach() {}
	void onDetach() {}
	void onUpdate(float timestep) {}
	void onEvent(proton::Event& event) {}
};

class Sandbox : public proton::Application
{
public:

	bool onCreate() override
	{
		LOG_OK("Hello from SandBox :)");
		pushLayer(new MainLayer());
		pushOverlay(new proton::ImGuiLayer());
		return true;
	}

};

proton::Application* proton::createApp() {
	return new Sandbox();
}