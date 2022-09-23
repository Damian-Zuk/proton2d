#include <Proton2D.h>

#include "TestLayer.h"

class Sandbox : public proton::Application
{
public:
	Sandbox(const std::string& name) : proton::Application(name) {}

	bool OnCreate() override
	{
		PushLayer(new TestLayer());
		return true;
	}
};

int main(int argc, char** argv) 
{
	auto app = new Sandbox("Proton Engine");
	app->Run();
	delete app;
}