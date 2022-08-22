#pragma once

#include <memory>

#ifdef PROTON_PLATFORM_WINDOWS

#else
#error Unsuportted platform!
#endif

#define BIND_EVENT_FN(x) std::bind(&x, this, std::placeholders::_1)

namespace proton 
{
	template <typename T>
	using Shared = std::shared_ptr<T>;

	template <typename T>
	using Unique = std::unique_ptr<T>;

	template <typename T, class... Types>
	constexpr Shared<T> createShared(Types&&... args)
	{
		return std::make_shared<T>(std::forward<Types>(args)...);
	}

	template <typename T, class... Types>
	constexpr Unique<T> createUnique(Types&&... args)
	{
		return std::make_unique<T>(std::forward<Types>(args)...);
	}
}