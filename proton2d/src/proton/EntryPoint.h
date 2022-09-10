#pragma once

#ifdef PROTON_PLATFORM_WINDOWS

extern proton::Application* proton::CreateApp();

int main(int argc, char** argv) {
	auto app = proton::CreateApp();
	app->Run();
	delete app;
}

#endif // PROTON_PLATFORM_WINDOWS
