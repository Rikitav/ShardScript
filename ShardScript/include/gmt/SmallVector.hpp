#pragma once
#include <shard/Definitions.hpp>

#include <gmt/Allocator.hpp>
#include <gmt/Containers.hpp>
#include <gmt/Ref.hpp>

#include <cstddef>
#include <cstring>
#include <span>
#include <type_traits>

namespace gmt
{
	/// <summary>
	/// Inline-first vector with no allocator machinery.
	/// The first Capacity elements live in the object, and only an overflow touches the heap.
	/// Meant for short, hot-lived argument and target lists where constructing a polymorphic allocator would cost more than the entries themselves.
	/// </summary>
	/// <typeparam name="T"></typeparam>
	/// <typeparam name="Capacity"></typeparam>
	template<typename T, std::size_t Capacity>
	class small_vector
	{
		static_assert(std::is_trivially_copyable_v<T>, "small_vector elements must be trivially copyable");
		static_assert(Capacity > 0, "small_vector needs at least one inline element");

		alignas(T) std::byte m_buffer[Capacity * sizeof(T)];
		T* m_data;
		std::size_t m_size = 0;
		std::size_t m_capacity = Capacity;

		T* inline_data()
		{
			return reinterpret_cast<T*>(m_buffer);
		}

		void grow(std::size_t needed)
		{
			std::size_t capacity = m_capacity * 2;
			if (capacity < needed)
				capacity = needed;

			T* fresh = static_cast<T*>(gmt::mem_alloc(capacity * sizeof(T)));
			if (m_size > 0)
				std::memcpy(fresh, m_data, m_size * sizeof(T));

			if (m_data != inline_data())
				gmt::mem_free(m_data);

			m_data = fresh;
			m_capacity = capacity;
		}

	public:
		small_vector()
			: m_data(reinterpret_cast<T*>(m_buffer))
		{ }

		~small_vector()
		{
			if (m_data != inline_data())
				gmt::mem_free(m_data);
		}

		small_vector(const small_vector&) = delete;
		small_vector& operator=(const small_vector&) = delete;

		SHARD_FORCE_INLINE void push_back(const T& value)
		{
			if (m_size == m_capacity)
				grow(m_size + 1);

			m_data[m_size++] = value;
		}

		SHARD_FORCE_INLINE std::size_t size() const { return m_size; }
		SHARD_FORCE_INLINE bool empty() const { return m_size == 0; }

		SHARD_FORCE_INLINE T* data() { return m_data; }
		SHARD_FORCE_INLINE const T* data() const { return m_data; }

		SHARD_FORCE_INLINE const T& operator[](std::size_t index) const { return m_data[index]; }
		SHARD_FORCE_INLINE T& operator[](std::size_t index) { return m_data[index]; }

		SHARD_FORCE_INLINE std::span<T> span() { return std::span<T>(m_data, m_size); }
		SHARD_FORCE_INLINE std::span<const T> span() const { return std::span<const T>(m_data, m_size); }

		SHARD_FORCE_INLINE T& back() { return m_data[m_size - 1]; }
		SHARD_FORCE_INLINE const T& back() const { return m_data[m_size - 1]; }

		SHARD_FORCE_INLINE void clear() { m_size = 0; }
	};
}