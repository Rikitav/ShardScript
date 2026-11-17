#include <shard/lexical/LexicalBuffer.hpp>
#include <shard/lexical/LexicalAnalyzer.hpp>
#include <shard/lexical/SourceProvider.hpp>

#include <shard/parsing/TextLocation.hpp>
#include <shard/parsing/SyntaxToken.hpp>
#include <shard/parsing/TokenType.hpp>

#include <gmt/Containers.hpp>

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

LexicalBuffer LexicalBuffer::from(gmt::vector<SyntaxToken> fromvector)
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

void LexicalBuffer::populate_from(gmt::vector<SyntaxToken> fromvector)
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

void LexicalBuffer::set_sequence(gmt::vector<SyntaxToken> setvector)
{
	clear();
	m_sequence = setvector;
}

void LexicalBuffer::set_index(std::size_t newIndex)
{
	m_currentIndex = newIndex;
	m_pending.clear();
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

gmt::vector<SyntaxToken>::iterator LexicalBuffer::begin()
{
	return m_sequence.begin();
}

gmt::vector<SyntaxToken>::iterator LexicalBuffer::end()
{
	return m_sequence.end();
}

gmt::vector<SyntaxToken>::const_iterator LexicalBuffer::begin() const
{
	return m_sequence.begin();
}

gmt::vector<SyntaxToken>::const_iterator LexicalBuffer::end() const
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
	m_pending.clear();
}

SyntaxToken LexicalBuffer::current()
{
	if (!m_pending.empty())
		return m_pending.back();

	if (m_currentIndex >= m_sequence.size())
		return SyntaxToken(TokenType::EndOfFile, L"", TextLocation());

	return m_sequence[m_currentIndex];
}

SyntaxToken LexicalBuffer::consume()
{
	if (!m_pending.empty())
	{
		SyntaxToken token = m_pending.back();
		m_pending.pop_back();
		return token;
	}

	if (m_currentIndex >= m_sequence.size())
		return SyntaxToken(TokenType::EndOfFile, L"", TextLocation());

	SyntaxToken token = m_sequence[m_currentIndex];
	m_currentIndex += 1;
	return token;
}

SyntaxToken LexicalBuffer::peek(int index)
{
	// pending tokens are read newest-first, ahead of the sequence
	const std::int64_t pendingCount = static_cast<std::int64_t>(m_pending.size());
	const std::int64_t target = static_cast<std::int64_t>(index) + 1;

	if (target < pendingCount)
		return m_pending[m_pending.size() - 1 - static_cast<std::size_t>(target)];

	const std::int64_t sequenceIndex = static_cast<std::int64_t>(m_currentIndex) + (target - pendingCount);
	if (sequenceIndex < 0 || sequenceIndex >= static_cast<std::int64_t>(m_sequence.size()))
		return SyntaxToken(TokenType::EndOfFile, L"", TextLocation());

	return m_sequence[static_cast<std::size_t>(sequenceIndex)];
}

void LexicalBuffer::put_back(SyntaxToken token)
{
	m_pending.push_back(token);
}

bool LexicalBuffer::can_consume()
{
	return !m_pending.empty() || m_currentIndex < m_sequence.size();
}

bool LexicalBuffer::can_peek()
{
	if (!m_pending.empty())
		return true;

	return m_currentIndex + 1 < m_sequence.size();
}
