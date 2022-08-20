#include <Proton2D.h>

using namespace proton;

class MainLayer : public proton::Layer {
public:

	MainLayer() : Layer("MainLayer") {}
	~MainLayer() {}

	void onAttach() {}
	void onDetach() {}
	void onUpdate(float timestep) {
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