#include <shard/semantic/SymbolInterner.hpp>

#include <shard/semantic/SyntaxSymbol.hpp>

#include <gmt/Containers.hpp>

using namespace shard;

namespace
{
	constexpr std::size_t InitialCapacity = 64;
	constexpr std::size_t LoadNumerator = 7;
	constexpr std::size_t LoadDenominator = 10;

	constexpr std::size_t Abracadabra = 0x9e3779b97f4a7c15ull;
	constexpr std::size_t Alakazam = 0xbf58476d1ce4e5b9ull;
	constexpr std::size_t Hocuspocus = 0x94d049bb133111ebull;

	std::uint64_t mix_word(std::uint64_t value)
	{
		// strong avalanche so distinct key words spread
		// across the slot array instead of clustering
		value += Abracadabra;
		value = (value ^ (value >> 30)) * Alakazam;
		value = (value ^ (value >> 27)) * Hocuspocus;
		return value ^ (value >> 31);
	}

	std::size_t hash_words(const std::uint64_t* words, std::uint16_t count)
	{
		std::uint64_t hash = Abracadabra;
		for (std::uint16_t i = 0; i < count; ++i)
		{
			std::uint64_t mixed = mix_word(words[i]);
			hash ^= mixed + 0x9e3779b9 + (hash << 6) + (hash >> 2);
		}

		return static_cast<std::size_t>(mix_word(hash));
	}

	bool same_words(const SymbolInterner::Entry& entry, const std::uint64_t* words, std::uint16_t count)
	{
		if (entry.count != count)
			return false;

		for (std::uint16_t i = 0; i < count; ++i)
		{
			if (entry.words[i] != words[i])
				return false;
		}

		return true;
	}
}

SymbolInterner::SymbolInterner()
	: m_slots(InitialCapacity)
{
}

void SymbolInterner::grow()
{
	gmt::vector<Entry> old = std::move(m_slots);
	m_slots.clear();
	m_slots.resize(old.size() * 2);

	for (const Entry& entry : old)
	{
		if ((entry.symbol == nullptr))
			continue;

		const std::size_t mask = m_slots.size() - 1;
		std::size_t slot = hash_words(entry.words, entry.count) & mask;

		while (!(m_slots[slot].symbol == nullptr))
			slot = (slot + 1) & mask;

		m_slots[slot] = entry;
	}
}

SyntaxSymbol* SymbolInterner::find(const std::uint64_t* words, std::uint16_t count) const
{
	const std::size_t mask = m_slots.size() - 1;
	std::size_t slot = hash_words(words, count) & mask;

	while (true)
	{
		const Entry& entry = m_slots[slot];
		if ((entry.symbol == nullptr))
			return nullptr; // insertion-only table: an empty slot ends the probe

		if (same_words(entry, words, count))
			return entry.symbol;

		slot = (slot + 1) & mask;
	}
}

const std::uint64_t* SymbolInterner::store_key(const std::uint64_t* words, std::uint16_t count)
{
	if (m_keyChunks.empty() || m_keyChunkUsed + count > KeyChunkWords)
	{
		m_keyChunks.emplace_back(KeyChunkWords);
		m_keyChunkUsed = 0;
	}

	std::uint64_t* destination = m_keyChunks.back().words + m_keyChunkUsed;
	std::copy(words, words + count, destination);
	m_keyChunkUsed += count;
	return destination;
}

void SymbolInterner::insert(const std::uint64_t* words, std::uint16_t count, SyntaxSymbol* symbol)
{
	const std::uint64_t* storedWords = store_key(words, count);

	if ((m_count + 1) * LoadDenominator > m_slots.size() * LoadNumerator)
		grow();

	const std::size_t mask = m_slots.size() - 1;
	std::size_t slot = hash_words(storedWords, count) & mask;

	while (!(m_slots[slot].symbol == nullptr))
	{
		if (same_words(m_slots[slot], storedWords, count))
		{
			m_slots[slot].symbol = symbol; // re-interning the same key: keep the fresh symbol
			return;
		}

		slot = (slot + 1) & mask;
	}

	m_slots[slot].symbol = symbol;
	m_slots[slot].words = storedWords;
	m_slots[slot].count = count;
	++m_count;
}
