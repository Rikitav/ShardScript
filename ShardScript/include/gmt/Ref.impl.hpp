#pragma once
#include <gmt/Ref.hpp>
#include <gmt/Arena.hpp>

#include <stdexcept>
#include <cstdint>

namespace gmt
{
	template<typename T>
	Ref<T>::Ref() :
		m_arena(nullptr),
		m_offset(0)
	{ }

	template<typename T>
	inline Ref<T>::Ref(nullref_t) :
		Ref()
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
		T* ptr = as_ptr();
		if (ptr == nullptr)
			throw std::runtime_error("Null pointer dereference");

		return *ptr;
	}

	template<typename T>
	inline T* Ref<T>::operator->() const
	{
		return as_ptr();
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
		if (is_null())
			throw std::runtime_error("Null pointer dereference");

		return *m_arena;
	}

	template<typename T>
	inline T* Ref<T>::as_ptr() const
	{
		if (is_null())
			return nullptr;

		return reinterpret_cast<T*>(m_arena->get_buffer() + m_offset);
	}

	template<typename T>
	template<typename U>
	inline bool Ref<T>::operator==(const Ref<U>& other) const
	{
		if (is_null() && other.is_null())
			return true;

		return m_arena == other.m_arena && m_offset == other.m_offset;
	}

	template<typename T>
	template<typename U>
	inline bool Ref<T>::operator!=(const Ref<U>& other) const
	{
		return !(*this == other);
	}
}

namespace std
{
	template<typename T>
	struct hash<gmt::Ref<T>>
	{
		std::size_t operator()(const gmt::Ref<T>& ref) const noexcept
		{
			return ref.m_offset ^ reinterpret_cast<std::size_t>(ref.m_arena);
		}
	};
}

#include <gmt/Ref.impl.hpp>
