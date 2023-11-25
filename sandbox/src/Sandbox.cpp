#include "pcheader.h"
#include "GameLayer.h"

class Sandbox : public proton::Application
{
public:
	virtual bool OnCreate() override
	{
		PushLayer(new GameLayer());
		return true;
	}
};

PROTON_APPLICATION_ENTRY_POINT(Sandbox);
