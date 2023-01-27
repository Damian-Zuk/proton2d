#include "pcheader.h"
#include "GameLayer.h"

class Sandbox : public proton::Application
{
public:
	Sandbox(const std::string& name) 
		: proton::Application(name) {}

	virtual bool OnCreate() override
	{
		PushLayer(new GameLayer());
		return true;
	}
};

PROTON_APPLICATION_ENTRY_POINT(Sandbox, "Proton Engine");
