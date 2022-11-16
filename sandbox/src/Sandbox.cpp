#include "GameLayer.h"

class Sandbox : public proton::Application
{
public:
	Sandbox(const std::string& name) 
		: proton::Application(name) {}

	virtual bool OnCreate() override
	{
		GetWindow().SetVSync(false);
		PushLayer(new GameLayer());
		return true;
	}
};

int main(int argc, char** argv) 
{
	Sandbox app("Proton Engine");
	app.Run();
}