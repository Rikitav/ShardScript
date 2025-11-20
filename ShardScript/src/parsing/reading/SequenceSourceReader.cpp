#include <shard/parsing/reading/SequenceSourceReader.h>
#include <shard/parsing/reading/SourceReader.h>
#include <shard/parsing/analysis/TextLocation.h>

#include <shard/syntax/SyntaxToken.h>
#include <shard/syntax/TokenType.h>

#include <string>
#include <vector>

using namespace shard::syntax;
using namespace shard::parsing;
using namespace shard::parsing::analysis;

SequenceSourceReader::SequenceSourceReader()
	: Sequence(), CurrentIndex(0) { }

SequenceSourceReader SequenceSourceReader::BufferFrom(SourceReader& reader)
{
	SequenceSourceReader sequenceReader;
	while (reader.CanConsume())
	{
		sequenceReader.Push(reader.Current());
		reader.Consume();
	}

	return sequenceReader;
}

void SequenceSourceReader::PopulateFrom(SourceReader& reader)
{
	while (reader.CanConsume())
	{
		Push(reader.Current());
		reader.Consume();
	}
}

void SequenceSourceReader::Push(SyntaxToken token)
{
	Sequence.push_back(token);
}

void SequenceSourceReader::Populate(std::vector<SyntaxToken> fromvector)
{
	for (const SyntaxToken& fromToken : fromvector)
		Push(fromToken);
}

void SequenceSourceReader::SetSequence(std::vector<SyntaxToken> setvector)
{
	Clear();
	Sequence = setvector;
}

void SequenceSourceReader::SetIndex(size_t newIndex)
{
	CurrentIndex = newIndex;
}

SyntaxToken SequenceSourceReader::At(size_t index)
{
	return Sequence.at(index);
}

SyntaxToken SequenceSourceReader::Front()
{
	return Sequence.front();
}

SyntaxToken SequenceSourceReader::Back()
{
	return Sequence.back();
}

size_t SequenceSourceReader::Size()
{
	return Sequence.size();
}

void SequenceSourceReader::Clear()
{
	CurrentIndex = 0;
	Sequence.clear();
}

SyntaxToken SequenceSourceReader::Current()
{
	if (CurrentIndex >= Sequence.size())
		return SyntaxToken(TokenType::EndOfFile, L"", TextLocation());

	return Sequence[CurrentIndex];
}

SyntaxToken SequenceSourceReader::Consume()
{
	CurrentIndex += 1;
	return Current();
}

SyntaxToken SequenceSourceReader::Peek(int index)
{
	return Sequence[CurrentIndex + 1 + index];
}

bool SequenceSourceReader::CanConsume()
{
	return CurrentIndex < Sequence.size();
}

bool SequenceSourceReader::CanPeek()
{
	return CurrentIndex < Sequence.size() - 1;
}

TextLocation SequenceSourceReader::GetLocation(std::wstring& word)
{
	return TextLocation();
}

bool SequenceSourceReader::ReadNext()
{
	return false;
}

bool SequenceSourceReader::PeekNext()
{
	return false;
}
