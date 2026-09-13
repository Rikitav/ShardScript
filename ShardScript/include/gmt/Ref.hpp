#pragma once

#include <cstddef>
#include <type_traits>

namespace gmt
{
	class Arena;

	template<typename T>
	class Ref
	{
		template<typename U>
		friend class Ref;

		Arena* m_arena;
		std::size_t m_offset;

	public:
		Ref();
		Ref(Arena& arena, std::size_t offset);

		template<typename U> requires std::is_convertible_v<U*, T*>
		Ref(const Ref<U>& other);

		Ref(const Ref&) = default;
		Ref& operator=(const Ref&) = default;

		T& operator*() const;
		T* operator->() const;

		bool is_null() const;

		std::size_t get_offset() const;
		Arena& get_arena() const;

		T* get() const;
	};
}
