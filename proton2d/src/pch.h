#pragma once

#include <iostream>
#include <functional>
#include <memory>
#include <sstream>
#include <cassert>

#include <string>
#include <vector>
#include <array>
#include <map>
#include <unordered_map>

#if PROTON_PLATFORM_WINDOWS
#include <Windows.h>
#endif

#include "proton/Core/Logger.h"
#include "proton/Debug/Instrumentor.h"
