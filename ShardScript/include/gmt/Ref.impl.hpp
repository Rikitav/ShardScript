#pragma once
#include <gmt/Ref.hpp>
#include <gmt/Arena.hpp>

namespace gmt
{
	template<typename T>
	Ref<T>::Ref() :
		m_arena(nullptr),
		m_offset(0)
	{ }

	template<typename T>
	Ref<T>::Ref(
		Arena& arena,
		std::size_t offset
	) :
		m_arena(&arena),
		m_offset(offset)
	{ }

	template<typename T>
	template<typename U> requires std::is_convertible_v<U*, T*>
	inline Ref<T>::Ref(
		const Ref<U>& other
	) :
		m_arena(other.m_arena),
		m_offset(other.m_offset)
	{
	}

	template<typename T>
	inline T& Ref<T>::operator*() const
	{
		return *get();
	}

	template<typename T>
	inline T* Ref<T>::operator->() const
	{
		return get();
	}

	template<typename T>
	inline bool Ref<T>::is_null() const
	{
		return m_arena == nullptr;
	}

	template<typename T>
	inline std::size_t Ref<T>::get_offset() const
	{
		return m_offset;
	}

	template<typename T>
	inline Arena& Ref<T>::get_arena() const
	{
		return *m_arena;
	}

	template<typename T>
	inline T* Ref<T>::get() const
	{
		return reinterpret_cast<T*>(m_arena->get_buffer() + m_offset);
	}
}

#include <gmt/Ref.impl.hpp>
