#pragma once
#include "Proton/Core/Base.h"
#include "Proton/Core/Application.h"

extern proton::Application* proton::CreateApplication(proton::ApplicationCommandLineArgs args);

#ifdef PROTON_PLATFORM_WINDOWS

	#ifdef PROTON_DISTRIBUTION
	extern int __argc;
	extern char** __argv;
	int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hInstPrev, PSTR cmdline, int cmdshow)
	{
		proton::Logger::Init();
		auto app = proton::CreateApplication({ __argc, __argv });
		app->Run();
		delete app;
	}
	#else
	int main(int argc, char** argv)
	{
		proton::Logger::Init();
		auto app = proton::CreateApplication({ argc, argv });
		app->Run();
		delete app;
	}
	#endif

#endif