#pragma once
#include <gmt/InlineVector.hpp>

#include <gmt/Arena.hpp>

#include <algorithm>

namespace gmt
{
	template<typename T, std::size_t Capacity>
	inline_vector<T, Capacity>::inline_vector()
		: m_pool(m_buffer, sizeof(m_buffer))
		, m_list(&m_pool)
	{ }

	template<typename T, std::size_t Capacity>
	std::pmr::vector<T>& inline_vector<T, Capacity>::get_vector()
	{
		return m_list;
	}

	template<typename T, std::size_t Capacity>
	void inline_vector<T, Capacity>::push_back(const T& val)
	{
		m_list.push_back(val);
	}

	template<typename T, std::size_t Capacity>
	Span<T> inline_vector<T, Capacity>::commit_array(Arena& arena)
	{
		Span<T> span = arena.emplace_array<T>(m_list.size());
		std::copy(m_list.begin(), m_list.end(), span.data());
		return span;
	}
}
