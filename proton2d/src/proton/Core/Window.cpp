#include "pch.h"
#include "Window.h"

#ifdef PROTON_PLATFORM_WINDOWS
#include "proton/Platform/Windows/WindowsWindow.h"
#endif

namespace proton {

	Window* Window::create(const WindowProps& props) 
	{
#ifdef PROTON_PLATFORM_WINDOWS
		return new WindowsWindow(props);
#else
		return nullptr;
#endif 
	}

}