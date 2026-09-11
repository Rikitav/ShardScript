#include <shard/lexical/LexicalBuffer.hpp>
#include <shard/lexical/LexicalAnalyzer.hpp>
#include <shard/lexical/SourceProvider.hpp>

#include <shard/parsing/TextLocation.hpp>
#include <shard/parsing/SyntaxToken.hpp>
#include <shard/parsing/TokenType.hpp>

#include <string>
#include <vector>

using namespace shard;

LexicalBuffer LexicalBuffer::from(SourceProvider& reader)
{
	LexicalBuffer sequenceReader;
	while (reader.can_consume())
	{
		sequenceReader.push_back(reader.current());
		reader.consume();
	}

	return sequenceReader;
}

LexicalBuffer LexicalBuffer::from(SourceTextProvider& reader)
{
	LexicalAnalyzer lexer(reader);
	return LexicalBuffer::from(lexer);
}

LexicalBuffer LexicalBuffer::from(std::vector<SyntaxToken> fromvector)
{
	LexicalBuffer buffer;
	buffer.set_sequence(fromvector);
	return buffer;
}

void LexicalBuffer::populate_from(SourceProvider& reader)
{
	while (reader.can_consume())
	{
		push_back(reader.current());
		reader.consume();
	}
}

void LexicalBuffer::populate_from(SourceTextProvider& reader)
{
	LexicalAnalyzer lexer(reader);
	while (lexer.can_consume())
	{
		push_back(lexer.current());
		lexer.consume();
	}
}

void LexicalBuffer::populate_from(std::vector<SyntaxToken> fromvector)
{
	for (const SyntaxToken& fromToken : fromvector)
		push_back(fromToken);
}

void LexicalBuffer::push_back(SyntaxToken token)
{
	m_sequence.push_back(token);
}

bool LexicalBuffer::is_empty()
{
	return m_sequence.empty();
}

void LexicalBuffer::set_sequence(std::vector<SyntaxToken> setvector)
{
	clear();
	m_sequence = setvector;
}

void LexicalBuffer::set_index(std::size_t newIndex)
{
	m_currentIndex = newIndex;
}

SyntaxToken LexicalBuffer::at(std::size_t index)
{
	return m_sequence.at(index);
}

SyntaxToken LexicalBuffer::first()
{
	return m_sequence.front();
}

SyntaxToken LexicalBuffer::last()
{
	return m_sequence.back();
}

std::vector<SyntaxToken>::iterator LexicalBuffer::begin()
{
	return m_sequence.begin();
}

std::vector<SyntaxToken>::iterator LexicalBuffer::end()
{
	return m_sequence.end();
}

std::vector<SyntaxToken>::const_iterator LexicalBuffer::begin() const
{
	return m_sequence.begin();
}

std::vector<SyntaxToken>::const_iterator LexicalBuffer::end() const
{
	return m_sequence.end();
}

std::size_t LexicalBuffer::size()
{
	return m_sequence.size();
}

void LexicalBuffer::clear()
{
	m_currentIndex = 0;
	m_sequence.clear();
}

SyntaxToken LexicalBuffer::current()
{
	if (m_currentIndex >= m_sequence.size())
		return SyntaxToken(TokenType::EndOfFile, L"", TextLocation());

	return m_sequence[m_currentIndex];
}

SyntaxToken LexicalBuffer::consume()
{
	if (m_currentIndex >= m_sequence.size())
		return SyntaxToken(TokenType::EndOfFile, L"", TextLocation());

	SyntaxToken token = m_sequence[m_currentIndex];
	m_currentIndex += 1;
	return token;
}

SyntaxToken LexicalBuffer::peek(int index)
{
	std::size_t target = m_currentIndex + 1 + static_cast<std::size_t>(index);
	if (target >= m_sequence.size())
		return SyntaxToken(TokenType::EndOfFile, L"", TextLocation());

	return m_sequence[target];
}

void LexicalBuffer::put_back(SyntaxToken token)
{
	if (m_currentIndex > m_sequence.size())
		m_currentIndex = m_sequence.size();

	m_sequence.insert(m_sequence.begin() + m_currentIndex, token);
}

bool LexicalBuffer::can_consume()
{
	return m_currentIndex < m_sequence.size();
}

bool LexicalBuffer::can_peek()
{
	if (m_sequence.empty())
		return false;

	return m_currentIndex + 1 < m_sequence.size();
}
