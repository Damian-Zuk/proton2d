#include <Proton.h>
#include <Proton/Core/EntryPoint.h>

class Sandbox : public proton::Application
{
public:
	Sandbox(proton::ApplicationSpecification& spec) 
		: proton::Application(spec)
	{
	}
};

proton::Application* proton::CreateApplication(proton::ApplicationCommandLineArgs args)
{
	proton::ApplicationSpecification spec;
	spec.Name = "Proton2D: Sandbox App";
	spec.CommandLineArgs = args;

	return new Sandbox(spec);
}
