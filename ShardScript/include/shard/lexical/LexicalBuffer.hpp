#pragma once
#include <shard/Definitions.hpp>

#include <shard/lexical/SourceTextProvider.hpp>
#include <shard/lexical/SourceProvider.hpp>
#include <shard/parsing/TextLocation.hpp>
#include <shard/parsing/SyntaxToken.hpp>

#include <gmt/Containers.hpp>

#include <string>
#include <vector>
#include <cstdint>

namespace shard
{
	class SHARD_API LexicalBuffer : public SourceProvider
	{
	private:
		gmt::vector<SyntaxToken> m_sequence;
		std::size_t m_currentIndex = 0;

	public:
		LexicalBuffer() = default;
		virtual ~LexicalBuffer() = default;

		static LexicalBuffer from(SourceProvider& provider);
		static LexicalBuffer from(SourceTextProvider& reader);
		static LexicalBuffer from(gmt::vector<SyntaxToken> fromvector);

		void populate_from(SourceProvider& reader);
		void populate_from(SourceTextProvider& reader);
		void populate_from(gmt::vector<SyntaxToken> fromvector);
		
		void set_sequence(gmt::vector<SyntaxToken> setvector);
		void set_index(std::size_t newIndex);

		std::size_t size();
		SyntaxToken at(std::size_t index);
		void push_back(SyntaxToken token);
		bool is_empty();
		void clear();

		SyntaxToken first();
		SyntaxToken last();

		gmt::vector<SyntaxToken>::iterator begin();
		gmt::vector<SyntaxToken>::iterator end();

		gmt::vector<SyntaxToken>::const_iterator begin() const;
		gmt::vector<SyntaxToken>::const_iterator end() const;

		SyntaxToken current() override;
		SyntaxToken consume() override;
		SyntaxToken peek(int index) override;
		void put_back(SyntaxToken token) override;
		bool can_consume() override;
		bool can_peek() override;
	};
}
