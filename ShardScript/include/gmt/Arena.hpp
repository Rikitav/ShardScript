#pragma once
#include <shard/Definitions.hpp>

#include <gmt/Alignment.hpp>
#include <gmt/Ref.hpp>
#include <gmt/Span.hpp>

#include <cstddef>
#include <limits>
#include <new>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

namespace gmt::detail
{
	struct destructor_entry
	{
		std::size_t offset;
		void(*destroy)(void*);
	};

	static constexpr void* resolve(std::byte* m_buffer, std::size_t offset)
	{
		return m_buffer + offset;
	}

	template<typename T>
	static void destroy_object(void* ptr)
	{
		static_cast<T*>(ptr)->~T();
	}
}

namespace gmt
{
	class SHARD_API Arena final
	{
		static constexpr std::size_t initial_capacity = 16 * 1024;

		std::byte* m_buffer;
		std::size_t m_capacity;
		std::size_t m_cursor;
		std::vector<detail::destructor_entry> m_destructors;

	public:
		Arena();
		~Arena();

		Arena(const Arena&) = delete;
		Arena& operator=(const Arena&) = delete;

		Arena(Arena&&) = delete;
		Arena& operator=(Arena&&) = delete;

		template<typename T, typename... Args>
		inline Ref<T> emplace(Args&&... args);

		template<typename T>
		inline Span<T> allocate_array(std::size_t count);

		inline std::byte* get_buffer() const { return m_buffer; }
		inline std::size_t get_cursor() const { return m_cursor; }
		inline std::size_t get_capacity() const { return m_capacity; }

	private:
		void* allocate(std::size_t bytes);
		void grow(std::size_t required);
	};
}

// impl
namespace gmt
{
	template<typename T, typename... Args>
	inline Ref<T> Arena::emplace(Args&&... args)
	{
		static_assert(!std::is_abstract_v<T>, "arena object must not be abstract");
		static_assert(!std::is_copy_constructible_v<T>, "arena objects must not be copyable");
		static_assert(!std::is_copy_assignable_v<T>, "arena objects must not be copyable");

		std::size_t aligned = align_up(m_cursor, alignof(T));
		if (aligned + sizeof(T) > m_capacity)
			grow(aligned + sizeof(T));

		static_cast<void>(new (m_buffer + aligned) T(std::forward<Args>(args)...));
		m_cursor = aligned + sizeof(T);

		if constexpr (!std::is_trivially_destructible_v<T>)
			m_destructors.push_back(detail::destructor_entry{ aligned, &detail::destroy_object<T> });

		return Ref<T>(*this, aligned);
	}

	template<typename T>
	Span<T> Arena::allocate_array(std::size_t count)
	{
		static_assert(std::is_trivially_destructible_v<T>, "arena array elements must be trivially destructible");
		static_assert(std::is_trivially_copyable_v<T>, "arena array elements must be safe to relocate");

		if (count == 0)
			return Span<T>();

		if (count > std::numeric_limits<std::uint32_t>::max())
			throw std::length_error("arena array exceeds the 32-bit span capacity");

		std::size_t aligned = align_up(m_cursor, alignof(T));
		if (aligned + count * sizeof(T) > m_capacity)
			grow(aligned + count * sizeof(T));

		Span<T> span(*this, static_cast<std::uint32_t>(aligned), static_cast<std::uint32_t>(count));
		m_cursor = aligned + count * sizeof(T);
		return span;
	}
}

#include <gmt/Ref.impl.hpp>
#include <gmt/Span.impl.hpp>
