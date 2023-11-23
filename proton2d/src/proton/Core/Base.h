#pragma once

#include <memory>

#ifndef PROTON_DISTRIBUTION
	#define PT_EDITOR
#endif

#ifndef PROTON_PLATFORM_WINDOWS
	#error Unsuportted platform!
#endif

#ifdef PROTON_DEBUG
	#if defined(PROTON_PLATFORM_WINDOWS)
		#define PT_DEBUGBREAK() __debugbreak()
	#elif defined(PROTON_PLATFORM_LINUX)
		#include <signal.h>
		#define PT_DEBUGBREAK() raise(SIGTRAP)
	#else
		#error "Platform doesn't support debugbreak yet!"
	#endif
	#define PT_ENABLE_ASSERTS
#else
	#define PT_DEBUGBREAK()
	#define NDEBUG
#endif

#define PT_EXPAND_MACRO(x) x
#define PT_STRINGIFY_MACRO(x) #x

#define PT_BIND_FUNCTION(x) std::bind(&x, this, std::placeholders::_1)


namespace proton 
{
	template <typename T>
	using Shared = std::shared_ptr<T>;

	template <typename T>
	using Unique = std::unique_ptr<T>;

	template <typename T, class... Types>
	constexpr Shared<T> CreateShared(Types&&... args)
	{
		return std::make_shared<T>(std::forward<Types>(args)...);
	}

	template <typename T, class... Types>
	constexpr Unique<T> CreateUnique(Types&&... args)
	{
		return std::make_unique<T>(std::forward<Types>(args)...);
	}
}
