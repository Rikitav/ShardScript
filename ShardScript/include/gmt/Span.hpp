#pragma once

#include <gmt/NullRef.hpp>

#include <cstdint>
#include <span>
#include <type_traits>

namespace gmt
{
	class Arena;

	template<typename T>
	class Span
	{
		template<typename U>
		friend class Span;

		Arena* m_arena;
		std::uint32_t m_offset;
		std::uint32_t m_count;

	public:
		Span();
		Span(Arena& arena, std::uint32_t offset, std::uint32_t count);
		Span(nullref_t);

		template<typename U> requires std::is_convertible_v<U*, T*>
		Span(const Span<U>& other);

		std::uint32_t offset() const;
		std::uint32_t size() const;
		bool empty() const;

		T* data() const;
		std::span<T> get();
		std::span<const T> get() const;
	};
}
