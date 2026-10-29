#pragma once
#include <shard/Definitions.hpp>

#include <gmt/Alignment.hpp>
#include <gmt/Containers.hpp>
#include <gmt/Ref.hpp>
#include <span>

#include <cstddef>
#include <limits>
#include <new>
#include <optional>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

namespace gmt::detail
{
	// one record covers a run of contiguous same-type objects
	struct destructor_entry
	{
		std::size_t offset;
		std::uint32_t count;
		std::uint32_t type;
	};

	struct destructor_info
	{
		void (*destroy)(void*);
		std::size_t size;
	};

	template<typename T>
	static void destroy_object(void* ptr)
	{
		static_cast<T*>(ptr)->~T();
	}

	SHARD_API std::uint32_t register_destructor_type(void (*destroy)(void*), std::size_t size);
	SHARD_API destructor_info destructor_info_at(std::uint32_t type);

	template<typename T>
	std::uint32_t destructor_type_index()
	{
		static const std::uint32_t index = register_destructor_type(&destroy_object<T>, sizeof(T));
		return index;
	}

	inline void register_destructor(gmt::vector<destructor_entry>& entries, std::size_t offset, std::uint32_t type, std::size_t size, bool startedNewChunk)
	{
		if (!startedNewChunk && !entries.empty())
		{
			destructor_entry& last = entries.back();
			if (last.type == type && last.offset + static_cast<std::size_t>(last.count) * size == offset)
			{
				++last.count;
				return;
			}
		}

		entries.push_back(destructor_entry{ offset, 1, type });
	}

	static constexpr void* resolve(std::byte* m_buffer, std::size_t offset)
	{
		return m_buffer + offset;
	}
}

namespace gmt
{
	class SHARD_API Arena final
	{
		static constexpr std::size_t initial_capacity = 16 * 1024;

		gmt::vector<std::byte*> m_chunks;
		gmt::vector<std::size_t> m_chunkSizes;
		gmt::vector<std::size_t> m_chunkBases;
		std::size_t m_capacity;
		std::size_t m_cursor;
		gmt::vector<detail::destructor_entry> m_destructors;

	public:
		Arena();
		~Arena();

		Arena(const Arena&) = delete;
		Arena& operator=(const Arena&) = delete;

		Arena(Arena&&) = delete;
		Arena& operator=(Arena&&) = delete;

		template<typename T, typename... Args>
		inline T* emplace(Args&&... args);

		template<typename T>
		inline std::span<T> emplace_array(std::size_t count);

		template<typename T>
		inline std::optional<T*> ref_from_ptr(T* ptr);

		inline std::size_t get_cursor() const { return m_cursor; }
		inline std::size_t get_capacity() const { return m_capacity; }

		void* resolve(std::size_t offset) const;

	private:
		void grow(std::size_t required);
	};
}

// impl
namespace gmt
{
	template<typename T, typename... Args>
	inline T* Arena::emplace(Args&&... args)
	{
		static_assert(!std::is_abstract_v<T>, "arena object must not be abstract");
		static_assert(!std::is_copy_constructible_v<T>, "arena objects must not be copyable");
		static_assert(!std::is_copy_assignable_v<T>, "arena objects must not be copyable");
		static_assert(!std::is_move_constructible_v<T>, "arena objects must not be movable");
		static_assert(!std::is_move_assignable_v<T>, "arena objects must not be movable");

		std::size_t aligned = align_up(m_cursor, alignof(T));
		bool startedNewChunk = false;
		if (aligned + sizeof(T) > m_capacity)
		{
			// the object must not straddle a chunk boundary: the tail of the
			// previous chunk stays unused and the object lands in the new chunk
			grow(m_capacity + sizeof(T));
			aligned = align_up(m_chunkBases.back(), alignof(T));
			startedNewChunk = true;
		}

		// the cursor only moves forward and grow() only appends chunks, so the
		// new object always lands in the last chunk; resolve() is only needed
		// for offsets that may belong to older chunks
		void* address = m_chunks.back() + (aligned - m_chunkBases.back());
		static_cast<void>(new (address) T(std::forward<Args>(args)...));
		m_cursor = aligned + sizeof(T);

		if constexpr (!std::is_trivially_destructible_v<T>)
			detail::register_destructor(m_destructors, aligned, detail::destructor_type_index<T>(), sizeof(T), startedNewChunk);

		return static_cast<T*>(address);
	}

	template<typename T>
	std::span<T> Arena::emplace_array(std::size_t count)
	{
		static_assert(std::is_trivially_destructible_v<T>, "arena array elements must be trivially destructible");
		static_assert(std::is_trivially_copyable_v<T>, "arena array elements must be safe to relocate");

		if (count == 0)
			return std::span<T>();

		if (count > std::numeric_limits<std::uint32_t>::max())
			throw std::length_error("arena array exceeds the 32-bit span capacity");

		std::size_t aligned = align_up(m_cursor, alignof(T));
		if (aligned + count * sizeof(T) > m_capacity)
		{
			grow(m_capacity + count * sizeof(T));
			// see emplace: arrays must not straddle a chunk boundary either
			aligned = align_up(m_chunkBases.back(), alignof(T));
		}

		std::span<T> span(reinterpret_cast<T*>(m_chunks.back() + (aligned - m_chunkBases.back())), count);
		m_cursor = aligned + count * sizeof(T);
		return span;
	}

	template<typename T>
	std::optional<T*> Arena::ref_from_ptr(T* ptr)
	{
		if (ptr == nullptr)
			return std::nullopt;

		const std::byte* address = reinterpret_cast<const std::byte*>(ptr);
		for (std::size_t i = m_chunks.size(); i-- > 0; )
		{
			const std::byte* chunk = m_chunks[i];
			if (address < chunk || address >= chunk + m_chunkSizes[i])
				continue;

			return static_cast<T*>(ptr);
		}

		return std::nullopt;
	}
}
