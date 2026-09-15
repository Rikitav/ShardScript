#pragma once
#include <gmt/Arena.hpp>
#include <gmt/Span.hpp>

namespace gmt
{
	template<typename T>
	Span<T>::Span() :
		m_arena(nullptr),
		m_offset(0),
		m_count(0)
	{ }

	template<typename T>
	inline Span<T>::Span(nullref_t) :
		Span()
	{ }

	template<typename T>
	Span<T>::Span(
		Arena& arena,
		std::uint32_t offset,
		std::uint32_t count
	) :
		m_arena(&arena),
		m_offset(offset),
		m_count(count)
	{ }

	template<typename T>
	template<typename U> requires std::is_convertible_v<U*, T*>
	inline Span<T>::Span(
		const Span<U>& other
	) :
		m_arena(other.m_arena),
		m_offset(other.m_offset),
		m_count(other.m_count)
	{ }

	template<typename T>
	inline std::uint32_t Span<T>::offset() const
	{
		return m_offset;
	}

	template<typename T>
	inline std::uint32_t Span<T>::size() const
	{
		return m_count;
	}

	template<typename T>
	inline bool Span<T>::empty() const
	{
		return m_count == 0;
	}

	template<typename T>
	inline T* Span<T>::data() const
	{
		return reinterpret_cast<T*>(m_arena->get_buffer() + m_offset);
	}

	template<typename T>
	inline std::span<T> Span<T>::get()
	{
		if (m_count == 0)
			return std::span<T>();

		return std::span<T>(data(), m_count);
	}

	template<typename T>
	inline std::span<const T> Span<T>::get() const
	{
		if (m_count == 0)
			return std::span<const T>();

		return std::span<const T>(data(), m_count);
	}
}
