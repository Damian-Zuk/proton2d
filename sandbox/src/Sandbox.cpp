#include <Proton2D.h>

class Sandbox : public proton::Application
{
public:

	bool OnStart() override 
	{
		LOG_OK("Hello from SandBox :)");
		return true;
	}

};

proton::Application* proton::CreateApplication() {
	return new Sandbox();
}