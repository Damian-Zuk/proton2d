#include <Proton2D.h>

class Sandbox : public proton::Application
{
public:

	bool onCreate() override
	{
		LOG_OK("Hello from SandBox :)");
		return true;
	}

};

proton::Application* proton::createApp() {
	return new Sandbox();
}