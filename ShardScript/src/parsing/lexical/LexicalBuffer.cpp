#include <shard/parsing/lexical/LexicalBuffer.h>
#include <shard/parsing/lexical/LexicalAnalyzer.h>
#include <shard/parsing/lexical/SourceProvider.h>
#include <shard/parsing/analysis/TextLocation.h>

#include <shard/syntax/SyntaxToken.h>
#include <shard/syntax/TokenType.h>

#include <string>
#include <vector>

using namespace shard;

LexicalBuffer::LexicalBuffer() : Sequence(), CurrentIndex(0)
{

}

LexicalBuffer::~LexicalBuffer()
{
	Sequence.clear();
}

LexicalBuffer LexicalBuffer::From(SourceProvider& reader)
{
	LexicalBuffer sequenceReader;
	while (reader.CanConsume())
	{
		sequenceReader.Push(reader.Current());
		reader.Consume();
	}

	return sequenceReader;
}

LexicalBuffer LexicalBuffer::From(SourceTextProvider& reader)
{
	LexicalAnalyzer lexer(reader);
	return LexicalBuffer::From(lexer);
}

LexicalBuffer LexicalBuffer::From(std::vector<shard::SyntaxToken> fromvector)
{
	LexicalBuffer buffer;
	buffer.SetSequence(fromvector);
	return buffer;
}

void LexicalBuffer::PopulateFrom(SourceProvider& reader)
{
	while (reader.CanConsume())
	{
		Push(reader.Current());
		reader.Consume();
	}
}

void LexicalBuffer::PopulateFrom(SourceTextProvider& reader)
{
	LexicalAnalyzer lexer(reader);
	while (lexer.CanConsume())
	{
		Push(lexer.Current());
		lexer.Consume();
	}
}

void LexicalBuffer::PopulateFrom(std::vector<SyntaxToken> fromvector)
{
	for (const SyntaxToken& fromToken : fromvector)
		Push(fromToken);
}

void LexicalBuffer::Push(SyntaxToken token)
{
	Sequence.push_back(token);
}

void LexicalBuffer::SetSequence(std::vector<SyntaxToken> setvector)
{
	Clear();
	Sequence = setvector;
}

void LexicalBuffer::SetIndex(size_t newIndex)
{
	CurrentIndex = newIndex;
}

SyntaxToken LexicalBuffer::At(size_t index)
{
	return Sequence.at(index);
}

SyntaxToken LexicalBuffer::Front()
{
	return Sequence.front();
}

SyntaxToken LexicalBuffer::Back()
{
	return Sequence.back();
}

std::vector<SyntaxToken>::iterator LexicalBuffer::begin()
{
	return Sequence.begin();
}

std::vector<SyntaxToken>::iterator LexicalBuffer::end()
{
	return Sequence.end();
}

std::vector<SyntaxToken>::const_iterator LexicalBuffer::begin() const
{
	return Sequence.begin();
}

std::vector<SyntaxToken>::const_iterator LexicalBuffer::end() const
{
	return Sequence.end();
}

size_t LexicalBuffer::Size()
{
	return Sequence.size();
}

void LexicalBuffer::Clear()
{
	CurrentIndex = 0;
	Sequence.clear();
}

SyntaxToken LexicalBuffer::Current()
{
	if (CurrentIndex >= Sequence.size())
		return SyntaxToken(TokenType::EndOfFile, L"", TextLocation());

	return Sequence[CurrentIndex];
}

SyntaxToken LexicalBuffer::Consume()
{
	CurrentIndex += 1;
	return Current();
}

SyntaxToken LexicalBuffer::Peek(int index)
{
	return Sequence[CurrentIndex + 1 + index];
}

bool LexicalBuffer::CanConsume()
{
	return CurrentIndex < Sequence.size();
}

bool LexicalBuffer::CanPeek()
{
	return CurrentIndex < Sequence.size() - 1;
}
