#pragma once

#include <memory>

#define PROTON_ASSETS_DIR "assets/"
#define PROTON_ENGINE_ASSETS_DIR "../proton2d/assets"

#ifndef PROTON_PLATFORM_WINDOWS
#error Unsuportted platform!
#endif

#ifndef PROTON_DEBUG
#define NDEBUG
#endif

#ifndef PROTON_DISTRIBUTION
#define PROTON_EDITOR 1
#else
#define PROTON_EDITOR 0
#endif

#define BIND_FUNCTION(x) std::bind(&x, this, std::placeholders::_1)

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
