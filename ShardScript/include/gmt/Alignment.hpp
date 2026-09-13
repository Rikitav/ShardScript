#pragma once

#include <type_traits>

namespace gmt
{
	template<typename T> requires std::is_integral_v<T>
	[[nodiscard]] constexpr T align_up(T value, T alignment)
	{
		return (value + alignment - 1) / alignment * alignment;
	}

	template<typename T> requires std::is_integral_v<T>
	[[nodiscard]] constexpr T align_down(T value, T alignment)
	{
		return value / alignment * alignment;
	}

	template<typename T> requires std::is_integral_v<T>
	[[nodiscard]] constexpr bool is_aligned(T value, T alignment)
	{
		return value % alignment == 0;
	}
}
