#pragma once

#include <gmt/NullRef.hpp>

#include <cstddef>
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
	class Ref
	{
		template<typename U>
		friend class Ref;

		template<typename>
		friend struct ::std::hash;

		Arena* m_arena;
		std::size_t m_offset;

	public:
		Ref();
		Ref(Arena& arena, std::size_t offset);
		Ref(nullref_t);

		template<typename U> requires std::is_convertible_v<U*, T*>
		Ref(const Ref<U>& other);
		
		template<typename U> requires std::is_base_of_v<T, U>
		Ref<U> as() const;
		
		Ref(const Ref&) = default;
		Ref& operator=(const Ref&) = default;

		T& operator*() const;
		T* operator->() const;

		bool is_null() const;

		template<typename U>
		bool operator==(const Ref<U>& other) const;

		template<typename U>
		bool operator!=(const Ref<U>& other) const;

		std::size_t get_offset() const;
		Arena& get_arena() const;

		T* as_ptr() const;
	};
}
