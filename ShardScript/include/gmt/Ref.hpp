#pragma once

#include <gmt/NullRef.hpp>

#include <cstddef>

namespace gmt
{
	// Arena objects are addressed with raw pointers. Chunks are never
	// relocated once allocated, so a pointer remains valid for the arena's
	// lifetime; the old offset-based Ref type is gone.

	// Trusted downcast matching the historical Ref::as<>() behaviour; the
	// caller has already checked the dynamic kind, and constness is dropped
	// exactly as the offset-based handle did.
	template<typename U, typename T>
	[[nodiscard]] constexpr U* as(T* ptr) noexcept
	{
		return const_cast<U*>(static_cast<const U*>(ptr));
	}
}
