#pragma once
#include <gmt/Ref.hpp>
#include <gmt/Span.hpp>

#include <cstddef>
#include <memory_resource>
#include <vector>

namespace gmt
{
	// a vector that allocates from an inline buffer first and spills onto the heap only on overflow
	template<typename T, std::size_t Capacity>
	class inline_vector
	{
		alignas(T) std::byte m_buffer[Capacity * sizeof(T)];

		std::pmr::monotonic_buffer_resource m_pool;
		std::pmr::vector<T> m_list;

	public:
		inline_vector();
		inline_vector(const inline_vector& other) = delete;
		inline_vector& operator=(const inline_vector& other) = delete;

		std::pmr::vector<T>& get_vector();
		void push_back(const T& val);

		Span<T> commit_array(Arena& arena);
	};

	template<typename T, std::size_t Capacity>
	using inline_ref_vector = inline_vector<Ref<T>, Capacity>;
}

#include <gmt/InlineVector.impl.hpp>
