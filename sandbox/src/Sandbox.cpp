#include <Proton.h>

class Sandbox : public proton::Application
{
public:
	virtual bool OnCreate() override { return true; }
};

PROTON_APPLICATION_ENTRY_POINT(Sandbox);
