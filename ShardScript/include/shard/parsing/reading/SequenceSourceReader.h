#pragma once
#include <shard/ShardScriptAPI.h>

#include <shard/parsing/reading/SourceReader.h>
#include <shard/parsing/analysis/TextLocation.h>
#include <shard/syntax/SyntaxToken.h>

#include <string>
#include <vector>

namespace shard::parsing
{
	class SHARD_API SequenceSourceReader : public SourceReader
	{
		std::vector<shard::syntax::SyntaxToken> Sequence;
		size_t CurrentIndex;

	public:
		SequenceSourceReader();
		virtual ~SequenceSourceReader();

		static SequenceSourceReader BufferFrom(SourceReader& reader);
		void PopulateFrom(SourceReader& reader);
		void Push(shard::syntax::SyntaxToken token);
		void Populate(std::vector<shard::syntax::SyntaxToken> fromvector);
		void SetSequence(std::vector<shard::syntax::SyntaxToken> setvector);
		void SetIndex(size_t newIndex);
		shard::syntax::SyntaxToken At(size_t index);
		shard::syntax::SyntaxToken Front();
		shard::syntax::SyntaxToken Back();
		size_t Size();
		void Clear();

		shard::syntax::SyntaxToken Current() override;
		shard::syntax::SyntaxToken Consume() override;
		shard::syntax::SyntaxToken Peek(int index) override;
		bool CanConsume() override;
		bool CanPeek() override;

	protected:
		shard::parsing::analysis::TextLocation GetLocation(std::wstring& word) override;
		bool ReadNext() override;
		bool PeekNext() override;
	};
}

