#include <shard/parsing/TextLocation.hpp>
#include <shard/parsing/SyntaxToken.hpp>

#include <string>

using namespace shard;

TextLocation::TextLocation() :
	m_fileName(),
	m_line(0),
	m_offset(0),
	m_length(0)
{ }

TextLocation::TextLocation(const SyntaxToken& left, const SyntaxToken& right) :
	TextLocation(left.get_location(), right.get_location())
{ }

TextLocation::TextLocation(const TextLocation& left, const SyntaxToken& right) :
	TextLocation(left, right.get_location())
{ }

TextLocation::TextLocation(const SyntaxToken& left, const TextLocation& right) :
	TextLocation(left.get_location(), right)
{ }

TextLocation::TextLocation(const TextLocation& left, const TextLocation& right)
{
	// a default-constructed location carries no information - take the other side
	if (left.m_line == 0 && left.m_offset == 0 && left.m_length == 0)
	{
		*this = right;
		return;
	}

	if (right.m_line == 0 && right.m_offset == 0 && right.m_length == 0)
	{
		*this = left;
		return;
	}

	if (left.m_fileName != right.m_fileName)
	{
		m_fileName = L"<INVALID>";
		m_line = -1;
		m_offset = -1;
		m_length = -1;
		return;
	}

	TextLocation from = left;
	TextLocation to = right;

	// normalize order so the span starts at the earlier location
	if (to.m_line < from.m_line || (to.m_line == from.m_line && to.m_offset < from.m_offset))
		std::swap(from, to);

	m_fileName = from.m_fileName;
	m_line = from.m_line;
	m_offset = from.m_offset;

	// note: offsets are line-relative, so the length of a span crossing
	// multiple lines is approximate
	std::int32_t start = from.m_offset;
	std::int32_t end = to.m_offset + to.m_length;

	m_length = end - start;
	if (m_length < 0)
		m_length = 0;
}

TextLocation::TextLocation(
	const std::wstring_view filename,
	std::int32_t line,
	std::int32_t offset,
	std::int32_t length
) :
	m_fileName(filename),
	m_line(line),
	m_offset(offset),
	m_length(length)
{ }
