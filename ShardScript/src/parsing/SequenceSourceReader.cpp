#include <shard/parsing/SequenceSourceReader.h>

using namespace std;
using namespace shard::syntax;
using namespace shard::syntax::analysis;
using namespace shard::parsing;

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

void SequenceSourceReader::Populate(vector<SyntaxToken> fromVector)
{
	for (const SyntaxToken& fromToken : fromVector)
		Push(fromToken);
}

void SequenceSourceReader::SetSequence(vector<SyntaxToken> setVector)
{
	Clear();
	Sequence = setVector;
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
		return SyntaxToken(TokenType::EndOfFile, "", TextLocation());

	return Sequence[CurrentIndex];
}

SyntaxToken SequenceSourceReader::Consume()
{
	CurrentIndex += 1;
	return Current();
}

SyntaxToken SequenceSourceReader::Peek()
{
	return Sequence[CurrentIndex + 1];
}

bool SequenceSourceReader::CanConsume()
{
	return CurrentIndex < Sequence.size();
}

bool SequenceSourceReader::CanPeek()
{
	return CurrentIndex < Sequence.size() - 1;
}

TextLocation SequenceSourceReader::GetLocation(string& word)
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
