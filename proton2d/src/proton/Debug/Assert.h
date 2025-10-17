// 
// Assert macros that trigger debugbreak
// From: https://github.com/TheCherno/Hazel/blob/master/Hazel/src/Hazel/Core/Assert.h
// 
#pragma once

#include "Proton/Core/Base.h"
#include "Proton/Debug/Logger.h"
#include <filesystem>

#ifdef PROTON_DEBUG
	#if defined(PROTON_PLATFORM_WINDOWS)
		#define PT_DEBUGBREAK() __debugbreak()
	#elif defined(PROTON_PLATFORM_LINUX)
		#include <signal.h>
		#define PT_DEBUGBREAK() raise(SIGTRAP)
	#endif
	#define PT_ENABLE_ASSERT
#else
	#define PT_DEBUGBREAK()
	#define NDEBUG
#endif

#define PT_ENABLE_VERIFY

#ifdef PT_ENABLE_ASSERT
// Alteratively we could use the same "default" message for both "WITH_MSG" and "NO_MSG" and
// provide support for custom formatting by concatenating the formatting string instead of having the format inside the default message
	#define PT_INTERNAL_ASSERT_IMPL(type, check, msg, ...) { if(!(check)) { PT##type##ERROR(msg, __VA_ARGS__); PT_DEBUGBREAK(); } }
	#define PT_INTERNAL_ASSERT_WITH_MSG(type, check, ...) PT_INTERNAL_ASSERT_IMPL(type, check, "Assertion failed: {0}", __VA_ARGS__)
	#define PT_INTERNAL_ASSERT_NO_MSG(type, check) PT_INTERNAL_ASSERT_IMPL(type, check, "Assertion '{0}' failed at {1}:{2}", PT_STRINGIFY_MACRO(check), std::filesystem::path(__FILE__).filename().string(), __LINE__)

	#define PT_INTERNAL_ASSERT_GET_MACRO_NAME(arg1, arg2, macro, ...) macro
	#define PT_INTERNAL_ASSERT_GET_MACRO(...) PT_EXPAND_MACRO( PT_INTERNAL_ASSERT_GET_MACRO_NAME(__VA_ARGS__, PT_INTERNAL_ASSERT_WITH_MSG, PT_INTERNAL_ASSERT_NO_MSG) )

	// Currently accepts at least the condition and one additional parameter (the message) being optional
	#define PT_ASSERT(...) PT_EXPAND_MACRO( PT_INTERNAL_ASSERT_GET_MACRO(__VA_ARGS__)(_, __VA_ARGS__) )
	#define PT_CORE_ASSERT(...) PT_EXPAND_MACRO( PT_INTERNAL_ASSERT_GET_MACRO(__VA_ARGS__)(_CORE_, __VA_ARGS__) )
#else
	#define PT_ASSERT(...)
	#define PT_CORE_ASSERT(...)
#endif

#ifdef PT_ENABLE_VERIFY
	// Alteratively we could use the same "default" message for both "WITH_MSG" and "NO_MSG" and
	// provide support for custom formatting by concatenating the formatting string instead of having the format inside the default message
	#define PT_INTERNAL_VERIFY_IMPL(type, check, msg, ...) { if(!(check)) { PT##type##ERROR(msg, __VA_ARGS__); PT_DEBUGBREAK(); } }
	#define PT_INTERNAL_VERIFY_WITH_MSG(type, check, ...) PT_INTERNAL_VERIFY_IMPL(type, check, "Verify failed: {0}", __VA_ARGS__)
	#define PT_INTERNAL_VERIFY_NO_MSG(type, check) PT_INTERNAL_VERIFY_IMPL(type, check, "Verify '{0}' failed at {1}:{2}", PT_STRINGIFY_MACRO(check), std::filesystem::path(__FILE__).filename().string(), __LINE__)

	#define PT_INTERNAL_VERIFY_GET_MACRO_NAME(arg1, arg2, macro, ...) macro
	#define PT_INTERNAL_VERIFY_GET_MACRO(...) PT_EXPAND_MACRO( PT_INTERNAL_VERIFY_GET_MACRO_NAME(__VA_ARGS__, PT_INTERNAL_VERIFY_WITH_MSG, PT_INTERNAL_VERIFY_NO_MSG) )

	// Currently accepts at least the condition and one additional parameter (the message) being optional
	#define PT_VERIFY(...) PT_EXPAND_MACRO( PT_INTERNAL_VERIFY_GET_MACRO(__VA_ARGS__)(_, __VA_ARGS__) )
	#define PT_CORE_VERIFY(...) PT_EXPAND_MACRO( PT_INTERNAL_VERIFY_GET_MACRO(__VA_ARGS__)(_CORE_, __VA_ARGS__) )
#else
	#define PT_VERIFY(...)
	#define PT_CORE_VERIFY(...)
#endif

#define PT_THROW_ERROR(...) \
	PT_CORE_ERROR(__VA_ARGS__); //PT_CORE_ASSERT(false);
