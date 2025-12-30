#pragma once
#include <shard/ShardScriptAPI.h>

#include <shard/parsing/reading/SourceReader.h>
#include <shard/parsing/analysis/TextLocation.h>
#include <shard/syntax/SyntaxToken.h>

#include <string>
#include <vector>

namespace shard
{
	class SHARD_API SequenceSourceReader : public SourceReader
	{
		std::vector<shard::SyntaxToken> Sequence;
		size_t CurrentIndex;

	public:
		SequenceSourceReader();
		virtual ~SequenceSourceReader();

		static SequenceSourceReader BufferFrom(SourceReader& reader);
		void PopulateFrom(SourceReader& reader);
		void Push(shard::SyntaxToken token);
		void Populate(std::vector<shard::SyntaxToken> fromvector);
		void SetSequence(std::vector<shard::SyntaxToken> setvector);
		void SetIndex(size_t newIndex);
		shard::SyntaxToken At(size_t index);
		shard::SyntaxToken Front();
		shard::SyntaxToken Back();
		size_t Size();
		void Clear();

		shard::SyntaxToken Current() override;
		shard::SyntaxToken Consume() override;
		shard::SyntaxToken Peek(int index) override;
		bool CanConsume() override;
		bool CanPeek() override;

	protected:
		shard::TextLocation GetLocation(std::wstring& word) override;
		bool ReadNext() override;
		bool PeekNext() override;
	};
}

