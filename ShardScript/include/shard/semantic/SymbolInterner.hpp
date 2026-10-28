#pragma once
#include <shard/Definitions.hpp>

#include <gmt/Containers.hpp>
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
			SyntaxSymbol* symbol = nullptr; // nullref marks an empty slot
			const std::uint64_t* words = nullptr;
			std::uint16_t count = 0;
		};

	private:
		static constexpr std::size_t KeyChunkWords = 512;

		// Chunk storage is allocated and released through gmt::mem_* because
		// header-defined containers may be destroyed by a different module
		// than the one that created them.
		struct KeyChunk
		{
			std::uint64_t* words = nullptr;

			KeyChunk() = default;

			explicit KeyChunk(std::size_t wordCount) :
				words(static_cast<std::uint64_t*>(gmt::mem_alloc(wordCount * sizeof(std::uint64_t))))
			{ }

			~KeyChunk() { gmt::mem_free(words); }

			KeyChunk(const KeyChunk&) = delete;
			KeyChunk& operator=(const KeyChunk&) = delete;

			KeyChunk(KeyChunk&& other) noexcept :
				words(other.words)
			{
				other.words = nullptr;
			}

			KeyChunk& operator=(KeyChunk&& other) noexcept
			{
				if (this != &other)
				{
					gmt::mem_free(words);
					words = other.words;
					other.words = nullptr;
				}

				return *this;
			}
		};

		gmt::vector<Entry> m_slots;
		std::size_t m_count = 0;

		// keys are copied into owned chunks: they must outlive every probe,
		// and chunk arrays never move once allocated
		gmt::vector<KeyChunk> m_keyChunks;
		std::size_t m_keyChunkUsed = 0;

		void grow();
		const std::uint64_t* store_key(const std::uint64_t* words, std::uint16_t count);

	public:
		SymbolInterner();
		~SymbolInterner() = default;

		SymbolInterner(const SymbolInterner&) = delete;
		SymbolInterner& operator=(const SymbolInterner&) = delete;

		SyntaxSymbol* find(const std::uint64_t* words, std::uint16_t count) const;
		void insert(const std::uint64_t* words, std::uint16_t count, SyntaxSymbol* symbol);
	};
}
