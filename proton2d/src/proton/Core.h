#pragma once

#ifdef PROTON_PLATFORM_WINDOWS
#ifdef PROTON_BUILD_DLL
#define PROTON_API __declspec(dllexport)
#else
#define PROTON_API __declspec(dllimport)
#endif
#else
#error Unsuportted platform!
#endif
