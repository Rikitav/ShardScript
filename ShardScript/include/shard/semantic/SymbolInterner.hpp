#pragma once
#include <shard/Definitions.hpp>

#include <gmt/Ref.hpp>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

namespace shard
{
	class SyntaxSymbol;

	// hash-consing interner for composite symbols
	// keys are word sequences compared in full on every probe,
	// so distinct types can never merge; insertion-only

	class SHARD_API SymbolInterner
	{
	public:
		struct Entry
		{
			gmt::Ref<SyntaxSymbol> symbol; // nullref marks an empty slot
			const std::uint64_t* words = nullptr;
			std::uint16_t count = 0;
		};

	private:
		static constexpr std::size_t KeyChunkWords = 512;

		std::vector<Entry> m_slots;
		std::size_t m_count = 0;

		// keys are copied into owned chunks: they must outlive every probe,
		// and chunk arrays never move once allocated
		std::vector<std::unique_ptr<std::uint64_t[]>> m_keyChunks;
		std::size_t m_keyChunkUsed = 0;

		void grow();
		const std::uint64_t* store_key(const std::uint64_t* words, std::uint16_t count);

	public:
		SymbolInterner();
		~SymbolInterner() = default;

		SymbolInterner(const SymbolInterner&) = delete;
		SymbolInterner& operator=(const SymbolInterner&) = delete;

		gmt::Ref<SyntaxSymbol> find(const std::uint64_t* words, std::uint16_t count) const;
		void insert(const std::uint64_t* words, std::uint16_t count, gmt::Ref<SyntaxSymbol> symbol);
	};
}
