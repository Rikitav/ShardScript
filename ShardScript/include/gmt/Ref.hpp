#pragma once
#include <shard/Definitions.hpp>

#include <cstddef>

namespace gmt
{
	template<typename U, typename T>
	[[nodiscard]] SHARD_FORCE_INLINE constexpr U* as(T* ptr) noexcept
	{
		return const_cast<U*>(static_cast<const U*>(ptr));
	}
}
