#include <shard/lexical/LexicalBuffer.hpp>
#include <shard/lexical/LexicalAnalyzer.hpp>
#include <shard/lexical/SourceProvider.hpp>
#include <shard/analysis/TextLocation.hpp>

#include <shard/parsing/SyntaxToken.hpp>
#include <shard/lexical/TokenType.hpp>

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

void LexicalBuffer::SetIndex(std::size_t newIndex)
{
	CurrentIndex = newIndex;
}

SyntaxToken LexicalBuffer::At(std::size_t index)
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

std::size_t LexicalBuffer::Size()
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
	std::size_t target = CurrentIndex + 1 + static_cast<std::size_t>(index);
	if (target >= Sequence.size())
		return SyntaxToken(TokenType::EndOfFile, L"", TextLocation());

	return Sequence[target];
}

void LexicalBuffer::PutBackToken(SyntaxToken token)
{
	if (CurrentIndex > Sequence.size())
		CurrentIndex = Sequence.size();

	Sequence.insert(Sequence.begin() + CurrentIndex, token);
}

bool LexicalBuffer::CanConsume()
{
	return CurrentIndex < Sequence.size();
}

bool LexicalBuffer::CanPeek()
{
	if (Sequence.empty())
		return false;

	return CurrentIndex + 1 < Sequence.size();
}
