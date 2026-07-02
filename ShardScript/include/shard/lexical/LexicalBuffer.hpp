#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/lexical/SourceTextProvider.hpp>
#include <shard/lexical/SourceProvider.hpp>
#include <shard/analysis/TextLocation.hpp>
#include <shard/parsing/SyntaxToken.hpp>

#include <string>
#include <vector>

namespace shard
{
	class SHARD_API LexicalBuffer : public SourceProvider
	{
	private:
		std::vector<shard::SyntaxToken> Sequence;
		std::size_t CurrentIndex;

	public:
		LexicalBuffer();
		virtual ~LexicalBuffer();

		static LexicalBuffer From(SourceProvider& provider);
		static LexicalBuffer From(SourceTextProvider& reader);
		static LexicalBuffer From(std::vector<shard::SyntaxToken> fromvector);

		void PopulateFrom(SourceProvider& reader);
		void PopulateFrom(SourceTextProvider& reader);
		void PopulateFrom(std::vector<shard::SyntaxToken> fromvector);
		
		void SetSequence(std::vector<shard::SyntaxToken> setvector);
		void SetIndex(std::size_t newIndex);

		std::size_t Size();
		shard::SyntaxToken At(std::size_t index);
		void Push(shard::SyntaxToken token);
		void Clear();

		shard::SyntaxToken Front();
		shard::SyntaxToken Back();

		std::vector<shard::SyntaxToken>::iterator begin();
		std::vector<shard::SyntaxToken>::iterator end();

		std::vector<shard::SyntaxToken>::const_iterator begin() const;
		std::vector<shard::SyntaxToken>::const_iterator end() const;

		shard::SyntaxToken Current() override;
		shard::SyntaxToken Consume() override;
		shard::SyntaxToken Peek(int index) override;
		void PutBackToken(shard::SyntaxToken token) override;
		bool CanConsume() override;
		bool CanPeek() override;
	};
}

