#pragma once

#include <memory>
#include <bitset>

#ifndef PROTON_DISTRIBUTION
	#define PT_EDITOR
#endif

#ifndef PROTON_PLATFORM_WINDOWS
	#error Unsuportted platform!
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
	constexpr Shared<T> MakeShared(Types&&... args)
	{
		return std::make_shared<T>(std::forward<Types>(args)...);
	}

	template <typename T, class... Types>
	constexpr Unique<T> MakeUnique(Types&&... args)
	{
		return std::make_unique<T>(std::forward<Types>(args)...);
	}

	template<typename T>
	constexpr bool EnumHasAnyFlags(T value, T flags)
	{
		return ((__underlying_type(T))value & (__underlying_type(T))flags) != 0;
	}

	template<typename T>
	constexpr bool EnumHasAllFlags(T value, T flags)
	{
		return ((__underlying_type(T))value & (__underlying_type(T))flags) == (__underlying_type(T))flags;
	}

	template<typename T>
	constexpr bool EnumHasNoneFlags(T value, T flags)
	{
		return ((__underlying_type(T))value & (__underlying_type(T))flags) == 0;
	}

	template<typename T>
	constexpr T EnumAddFlags(T value, T flags)
	{
		return (T)((__underlying_type(T))value | (__underlying_type(T))flags);
	}
}
