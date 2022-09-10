#include "pch.h"
#include "proton/Core/Window.h"

#ifdef PROTON_PLATFORM_WINDOWS
#include "proton/Platform/Windows/WindowsWindow.h"
#endif

namespace proton {

	Unique<Window> Window::Create(const WindowProps& props) 
	{
	#ifdef PROTON_PLATFORM_WINDOWS
		return CreateUnique<WindowsWindow>(props);
	#else
		return nullptr;
	#endif 
	}

}