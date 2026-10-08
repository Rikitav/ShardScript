#pragma once

#include <cstddef>
#include <utility>
#include <vector>

namespace gmt
{
	template<typename K, typename V>
	class FlatMap
	{
		std::vector<std::pair<K, V>> m_entries;

	public:
		using iterator = typename std::vector<std::pair<K, V>>::iterator;
		using const_iterator = typename std::vector<std::pair<K, V>>::const_iterator;

		FlatMap() = default;

		bool is_empty() const;
		std::size_t length() const;

		V* find(const K& key);
		const V* find(const K& key) const;

		iterator begin();
		const_iterator begin() const;
		iterator end();
		const_iterator end() const;

		bool try_emplace(const K& key, const V& value);
		bool try_emplace(const K& key, V&& value);

	private:
		typename iterator locate(const K& key);
		typename const_iterator locate(const K& key) const;
	};
}

#include <gmt/FlatMap.impl.hpp>
