#include <shard/lexical/StringStreamReader.hpp>
#include <shard/lexical/SourceProvider.hpp>
#include <shard/parsing/TextLocation.hpp>

#include <sstream>
#include <istream>
#include <string>
#include <codecvt>
#include <locale>

using namespace shard;

StringStreamReader::StringStreamReader(
	const std::wstring& name,
	std::wstringstream& source
) :
	SourceTextProvider(),
	m_name(name)
{
	m_stringStream = std::wstringstream(source.str());
	m_stringStream.imbue(source.getloc());
}

StringStreamReader::StringStreamReader(
	const std::wstring& name,
	const std::wstring& source
) :
	SourceTextProvider(),
	m_name(name)
{
	m_stringStream = std::wstringstream(source);
	m_stringStream.imbue(std::locale::classic());
}

StringStreamReader::StringStreamReader(
	const std::wstring& name,
	const wchar_t* source,
	std::size_t count
) :
	SourceTextProvider(),
	m_name(name)
{
	std::wstring srcStr = std::wstring(source, count);
	m_stringStream = std::wstringstream(srcStr);
	m_stringStream.imbue(std::locale::classic());
}

bool StringStreamReader::read_next(wchar_t& ch)
{
	wchar_t PeekSymbol = m_stringStream.peek();
	if (PeekSymbol == WEOF)
		return false;

	m_stringStream.get(ch);
	if (ch == WEOF)
		return false;

	return true;
}

bool StringStreamReader::peek_next(wchar_t& ch)
{
	ch = m_stringStream.peek();
	return ch != WEOF;
}

std::wstring StringStreamReader::get_name()
{
	return m_name;
}
