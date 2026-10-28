#pragma once
#include <gmt/Allocator.hpp>

#include <cstddef>
#include <cstring>
#include <deque>
#include <functional>
#include <list>
#include <map>
#include <memory>
#include <new>
#include <set>
#include <span>
#include <string>
#include <unordered_map>
#include <type_traits>
#include <unordered_set>
#include <utility>
#include <vector>

// Allocator-aware standard containers. These are the only container types that
// may appear in the public API: crossing a module boundary requires both sides
// to agree on the allocator, otherwise one module frees another module's
// memory. Non-owning views (string_view, span) stay std types.

namespace gmt
{
	template<typename T, typename Alloc = allocator<T>>
	using vector = std::vector<T, Alloc>;

	template<typename T, typename Alloc = allocator<T>>
	using deque = std::deque<T, Alloc>;

	template<typename T, typename Alloc = allocator<T>>
	using list = std::list<T, Alloc>;

	template<typename CharT, typename Traits = std::char_traits<CharT>, typename Alloc = allocator<CharT>>
	using basic_string = std::basic_string<CharT, Traits, Alloc>;

	using string = basic_string<char>;
	using wstring = basic_string<wchar_t>;

	template<typename Key, typename T, typename Compare = std::less<Key>, typename Alloc = allocator<std::pair<const Key, T>>>
	using map = std::map<Key, T, Compare, Alloc>;

	template<typename Key, typename T, typename Compare = std::less<Key>, typename Alloc = allocator<std::pair<const Key, T>>>
	using multimap = std::multimap<Key, T, Compare, Alloc>;

	template<typename Key, typename Compare = std::less<Key>, typename Alloc = allocator<Key>>
	using set = std::set<Key, Compare, Alloc>;

	template<typename Key, typename Compare = std::less<Key>, typename Alloc = allocator<Key>>
	using multiset = std::multiset<Key, Compare, Alloc>;

	template<typename Key, typename T, typename Hash = std::hash<Key>, typename Equal = std::equal_to<Key>, typename Alloc = allocator<std::pair<const Key, T>>>
	using unordered_map = std::unordered_map<Key, T, Hash, Equal, Alloc>;

	template<typename Key, typename T, typename Hash = std::hash<Key>, typename Equal = std::equal_to<Key>, typename Alloc = allocator<std::pair<const Key, T>>>
	using unordered_multimap = std::unordered_multimap<Key, T, Hash, Equal, Alloc>;

	template<typename Key, typename Hash = std::hash<Key>, typename Equal = std::equal_to<Key>, typename Alloc = allocator<Key>>
	using unordered_set = std::unordered_set<Key, Hash, Equal, Alloc>;

	template<typename Key, typename Hash = std::hash<Key>, typename Equal = std::equal_to<Key>, typename Alloc = allocator<Key>>
	using unordered_multiset = std::unordered_multiset<Key, Hash, Equal, Alloc>;

	namespace detail
	{
		constexpr std::size_t round_up(std::size_t value, std::size_t alignment)
		{
			return (value + alignment - 1) / alignment * alignment;
		}

		// Array unique_ptrs need the element count after construction; it is
		// stored in an aligned header immediately before the elements.
		template<typename T>
		constexpr std::size_t array_header_size = round_up(sizeof(std::size_t), alignof(T));
	}

	template<typename T>
	struct object_deleter
	{
		void operator()(T* ptr) const noexcept
		{
			if (ptr == nullptr)
				return;

			ptr->~T();
			mem_free(ptr);
		}
	};

	template<typename T>
	struct object_deleter<T[]>
	{
		void operator()(T* ptr) const noexcept
		{
			if (ptr == nullptr)
				return;

			const std::size_t header = detail::array_header_size<T>;
			std::byte* base = reinterpret_cast<std::byte*>(ptr) - header;

			std::size_t count = 0;
			std::memcpy(&count, base, sizeof(count));

			for (std::size_t i = count; i > 0; --i)
				ptr[i - 1].~T();

			mem_free(base);
		}
	};

	template<typename T>
	using unique_ptr = std::unique_ptr<T, object_deleter<T>>;

	template<typename T, typename... Args> requires (!std::is_array_v<T>)
	unique_ptr<T> make_unique(Args&&... args)
	{
		constexpr std::size_t alignment = alignof(T) > alignof(std::max_align_t) ? alignof(T) : alignof(std::max_align_t);
		void* memory = alignment > alignof(std::max_align_t)
			? mem_alloc_aligned(sizeof(T), alignment)
			: mem_alloc(sizeof(T));

		if (memory == nullptr)
			throw std::bad_alloc();

		try
		{
			return unique_ptr<T>(new (memory) T(std::forward<Args>(args)...));
		}
		catch (...)
		{
			mem_free(memory);
			throw;
		}
	}

	template<typename T> requires (std::is_array_v<T> && std::extent_v<T> == 0)
	unique_ptr<T> make_unique(std::size_t count)
	{
		using Element = std::remove_extent_t<T>;

		constexpr std::size_t alignment = alignof(Element) > alignof(std::size_t) ? alignof(Element) : alignof(std::size_t);
		const std::size_t header = detail::array_header_size<Element>;
		const std::size_t bytes = header + count * sizeof(Element);

		void* memory = alignment > alignof(std::max_align_t)
			? mem_alloc_aligned(bytes, alignment)
			: mem_alloc(bytes);

		if (memory == nullptr)
			throw std::bad_alloc();

		std::memcpy(memory, &count, sizeof(count));

		Element* elements = reinterpret_cast<Element*>(static_cast<std::byte*>(memory) + header);
		std::size_t constructed = 0;

		try
		{
			for (; constructed < count; ++constructed)
				new (elements + constructed) Element();
		}
		catch (...)
		{
			while (constructed > 0)
				elements[--constructed].~Element();

			mem_free(memory);
			throw;
		}

		return unique_ptr<T>(elements);
	}

	// Shared ownership stores the allocation policy in the control block, so
	// the plain std::shared_ptr type is already safe across modules.
	template<typename T>
	using shared_ptr = std::shared_ptr<T>;

	template<typename T>
	using weak_ptr = std::weak_ptr<T>;

	template<typename T, typename... Args>
	shared_ptr<T> make_shared(Args&&... args)
	{
		return std::allocate_shared<T>(allocator<T>(), std::forward<Args>(args)...);
	}

	template<typename T, typename... Args>
	shared_ptr<T> allocate_shared(Args&&... args)
	{
		return std::allocate_shared<T>(allocator<T>(), std::forward<Args>(args)...);
	}
}
