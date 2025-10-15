#pragma once
#include <shard/parsing/SourceReader.h>
#include <shard/syntax/analysis/TextLocation.h>
#include <shard/syntax/SyntaxToken.h>
#include <string>
#include <vector>

namespace shard::parsing
{
	class SequenceSourceReader : public SourceReader
	{
		std::vector<shard::syntax::SyntaxToken> Sequence;
		size_t CurrentIndex;

	public:
		SequenceSourceReader();

		static SequenceSourceReader BufferFrom(SourceReader& reader);
		void PopulateFrom(SourceReader& reader);
		void Push(shard::syntax::SyntaxToken token);
		void Populate(vector<shard::syntax::SyntaxToken> fromVector);
		void SetSequence(vector<shard::syntax::SyntaxToken> setVector);
		void SetIndex(size_t newIndex);
		shard::syntax::SyntaxToken At(size_t index);
		shard::syntax::SyntaxToken Front();
		shard::syntax::SyntaxToken Back();
		size_t Size();
		void Clear();

		shard::syntax::SyntaxToken Current() override;
		shard::syntax::SyntaxToken Consume() override;
		shard::syntax::SyntaxToken Peek() override;
		bool CanConsume() override;
		bool CanPeek() override;

	protected:
		shard::syntax::analysis::TextLocation GetLocation(std::string& word) override;
		bool ReadNext() override;
		bool PeekNext() override;
	};
}

