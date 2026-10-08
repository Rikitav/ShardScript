#pragma once
#include <gmt/FlatMap.hpp>

#include <algorithm>
#include <utility>

namespace gmt
{
	template<typename K, typename V>
	bool FlatMap<K, V>::is_empty() const
	{
		return m_entries.empty();
	}

	template<typename K, typename V>
	std::size_t FlatMap<K, V>::length() const
	{
		return m_entries.size();
	}

	template<typename K, typename V>
	auto FlatMap<K, V>::locate(const K& key) -> iterator
	{
		return std::lower_bound(m_entries.begin(), m_entries.end(), key,
			[](const std::pair<K, V>& entry, const K& candidate)
			{
				return entry.first < candidate;
			});
	}

	template<typename K, typename V>
	auto FlatMap<K, V>::locate(const K& key) const -> const_iterator
	{
		return std::lower_bound(m_entries.begin(), m_entries.end(), key,
			[](const std::pair<K, V>& entry, const K& candidate)
			{
				return entry.first < candidate;
			});
	}

	template<typename K, typename V>
	typename FlatMap<K, V>::iterator FlatMap<K, V>::begin()
	{
		return m_entries.begin();
	}

	template<typename K, typename V>
	typename FlatMap<K, V>::const_iterator FlatMap<K, V>::begin() const
	{
		return m_entries.begin();
	}

	template<typename K, typename V>
	typename FlatMap<K, V>::iterator FlatMap<K, V>::end()
	{
		return m_entries.end();
	}

	template<typename K, typename V>
	typename FlatMap<K, V>::const_iterator FlatMap<K, V>::end() const
	{
		return m_entries.end();
	}

	template<typename K, typename V>
	V* FlatMap<K, V>::find(const K& key)
	{
		iterator it = locate(key);
		if (it == m_entries.end() || !(it->first == key))
			return nullptr;

		return &it->second;
	}

	template<typename K, typename V>
	const V* FlatMap<K, V>::find(const K& key) const
	{
		const_iterator it = locate(key);
		if (it == m_entries.end() || !(it->first == key))
			return nullptr;

		return &it->second;
	}

	template<typename K, typename V>
	bool FlatMap<K, V>::try_emplace(const K& key, const V& value)
	{
		iterator it = locate(key);
		if (it != m_entries.end() && it->first == key)
			return false;

		m_entries.emplace(it, key, value);
		return true;
	}

	template<typename K, typename V>
	bool FlatMap<K, V>::try_emplace(const K& key, V&& value)
	{
		iterator it = locate(key);
		if (it != m_entries.end() && it->first == key)
			return false;

		m_entries.emplace(it, key, std::move(value));
		return true;
	}
}
