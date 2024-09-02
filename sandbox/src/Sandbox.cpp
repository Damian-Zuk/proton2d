#include <Proton.h>

#define GAME_NET_PROTOCOL_VERSION 1

class Sandbox : public proton::Application
{
public:
	virtual bool OnCreate() override 
	{
		proton::NetworkManager::SetGameProtocolVersion(GAME_NET_PROTOCOL_VERSION);
		return true;
	}
};

PROTON_APPLICATION_ENTRY_POINT(Sandbox);
