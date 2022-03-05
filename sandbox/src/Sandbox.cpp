#include <Proton2D.h>

class Sandbox : public proton::Application
{
public:
	Sandbox() {

	}

	~Sandbox() {

	}
};

proton::Application* proton::CreateApplication() {
	return new Sandbox();
}