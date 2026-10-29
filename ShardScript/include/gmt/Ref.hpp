#pragma once
#include <cstddef>

namespace gmt
{
	template<typename U, typename T>
	[[nodiscard]] constexpr U* as(T* ptr) noexcept
	{
		return const_cast<U*>(static_cast<const U*>(ptr));
	}
}
