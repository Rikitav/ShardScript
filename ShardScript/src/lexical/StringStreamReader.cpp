#include <shard/lexical/StringStreamReader.hpp>
#include <shard/lexical/SourceProvider.hpp>
#include <shard/parsing/TextLocation.hpp>

#include <gmt/Containers.hpp>

#include <sstream>
#include <string>

using namespace shard;

StringStreamReader::~StringStreamReader() = default;

StringStreamReader::StringStreamReader(
	const gmt::wstring& name,
	std::wstringstream& source
) :
	SourceTextProvider(),
	m_name(name),
	m_text(source.str())
{ }

StringStreamReader::StringStreamReader(
	const gmt::wstring& name,
	const gmt::wstring& source
) :
	SourceTextProvider(),
	m_name(name),
	m_text(source)
{ }

StringStreamReader::StringStreamReader(
	const gmt::wstring& name,
	string_t source,
	std::size_t count
) :
	SourceTextProvider(),
	m_name(name),
	m_text(source, count)
{ }

bool StringStreamReader::read_next(wchar_t& ch)
{
	if (m_index >= m_text.size())
		return false;

	ch = m_text[m_index++];
	return true;
}

bool StringStreamReader::peek_next(wchar_t& ch)
{
	if (m_index >= m_text.size())
		return false;

	ch = m_text[m_index];
	return true;
}

gmt::wstring StringStreamReader::get_name()
{
	return m_name;
}
