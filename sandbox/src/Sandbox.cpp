#include "GameLayer.h"

class Sandbox : public proton::Application
{
public:
	Sandbox(const std::string& name) : proton::Application(name) {}

	bool OnCreate() override
	{
		proton::Application::GetWindow().SetVSync(false);
		PushLayer(new GameLayer());
		return true;
	}
};

int main(int argc, char** argv) 
{
	Sandbox app("Proton Engine");
	app.Run();
}