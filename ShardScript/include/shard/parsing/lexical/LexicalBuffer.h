#pragma once
#include <shard/ShardScriptAPI.h>

#include <shard/parsing/lexical/reading/SourceTextProvider.h>
#include <shard/parsing/lexical/SourceProvider.h>
#include <shard/parsing/analysis/TextLocation.h>
#include <shard/syntax/SyntaxToken.h>

#include <string>
#include <vector>

namespace shard
{
	class SHARD_API LexicalBuffer : public SourceProvider
	{
	private:
		std::vector<shard::SyntaxToken> Sequence;
		size_t CurrentIndex;

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
		void SetIndex(size_t newIndex);

		size_t Size();
		shard::SyntaxToken At(size_t index);
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
		bool CanConsume() override;
		bool CanPeek() override;
	};
}

