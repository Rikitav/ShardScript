#pragma once
#include <shard/Definitions.hpp>

#include <cstddef>
#include <cstring>
#include <limits>
#include <memory>
#include <new>
#include <type_traits>

namespace gmt
{
	SHARD_API void* mem_alloc(std::size_t bytes);
	SHARD_API void* mem_alloc_aligned(std::size_t bytes, std::size_t alignment);
	SHARD_API void* mem_calloc(std::size_t count, std::size_t bytes);
	SHARD_API void* mem_realloc(void* block, std::size_t bytes);
	SHARD_API void mem_free(void* ptr);

	// STL allocator backed by the shared entry points above.
	template<typename T>
	struct allocator
	{
		using value_type = T;
		using size_type = std::size_t;
		using difference_type = std::ptrdiff_t;

		using propagate_on_container_copy_assignment = std::true_type;
		using propagate_on_container_move_assignment = std::true_type;
		using propagate_on_container_swap = std::true_type;
		using is_always_equal = std::true_type;

		allocator() noexcept = default;
		allocator(const allocator&) noexcept = default;

		template<typename U>
		allocator(const allocator<U>&) noexcept { }
		allocator& operator=(const allocator& other) = default;

		[[nodiscard]] T* allocate(std::size_t count)
		{
			if (count > max_size())
				throw std::bad_array_new_length();

			const std::size_t bytes = count * sizeof(T);
			void* block = alignof(T) > alignof(std::max_align_t)
				? mem_alloc_aligned(bytes, alignof(T))
				: mem_alloc(bytes);

			if (block == nullptr)
				throw std::bad_alloc();

			return static_cast<T*>(block);
		}

		void deallocate(T* ptr, std::size_t) noexcept
		{
			mem_free(ptr);
		}

		[[nodiscard]] std::size_t max_size() const noexcept
		{
			return (std::numeric_limits<std::size_t>::max)() / sizeof(T);
		}
	};

	template<typename T, typename U>
	bool operator==(const allocator<T>&, const allocator<U>&) noexcept
	{
		return true;
	}

	template<typename T, typename U>
	bool operator!=(const allocator<T>&, const allocator<U>&) noexcept
	{
		return false;
	}
}
