#pragma once

#include <gmt/NullRef.hpp>

#include <cstdint>
#include <span>
#include <type_traits>

namespace std
{
	template<typename>
	struct hash;
}

namespace gmt
{
	class Arena;

	template<typename T>
	class Span
	{
		template<typename U>
		friend class Span;

		template<typename>
		friend struct ::std::hash;

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

		template<typename U>
		bool operator==(const Span<U>& other) const;

		template<typename U>
		bool operator!=(const Span<U>& other) const;

		T* data() const;
		std::span<T> as_span();
		std::span<const T> as_span() const;
	};
}
