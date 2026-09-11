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

TextLocation::TextLocation(const SyntaxToken& left, const SyntaxToken& right)
{
	TextLocation from = left.get_location();
	TextLocation to = right.get_location();

	if (0 != wcscmp(from.m_fileName.c_str(), to.m_fileName.c_str()))
	{
		m_fileName = L"<INVALID>";
		m_line = -1;
		m_offset = -1;
		m_length = -1;
		return;
	}

	if ((to.m_line > from.m_line) || (to.m_offset > from.m_offset))
		std::swap(from, to);

	m_fileName = from.m_fileName;
	m_line = from.m_line;
	m_offset = from.m_offset;

	std::int32_t start = from.m_offset;
	std::int32_t end = to.m_offset + to.m_length;

	m_length = end - start;
	if (m_length < 0)
		m_length = 0;
}

TextLocation::TextLocation(
	const std::wstring& filename,
	std::int32_t line,
	std::int32_t offset,
	std::int32_t length
) :
	m_fileName(filename),
	m_line(line),
	m_offset(offset),
	m_length(length)
{ }
