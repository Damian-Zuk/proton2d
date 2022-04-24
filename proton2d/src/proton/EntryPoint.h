#pragma once

#ifdef PROTON_PLATFORM_WINDOWS

extern proton::Application* proton::createApp();

int main(int argc, char** argv) {
	auto app = proton::createApp();
	app->run();
	delete app;
}

#endif // PROTON_PLATFORM_WINDOWS
