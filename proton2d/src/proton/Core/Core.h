#pragma once

#ifdef PROTON_PLATFORM_WINDOWS

#else
#error Unsuportted platform!
#endif

#define BIND_EVENT_FN(x) std::bind(&x, this, std::placeholders::_1)