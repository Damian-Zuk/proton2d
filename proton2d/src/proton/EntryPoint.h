#pragma once

#ifdef PROTON_PLATFORM_WINDOWS

extern proton::Application* proton::CreateApplication();

int main(int argc, char** argv) {
	auto app = proton::CreateApplication();
	app->Start();
	delete app;
}

#endif // PROTON_PLATFORM_WINDOWS
