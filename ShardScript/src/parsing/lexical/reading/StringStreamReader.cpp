#include <shard/parsing/lexical/reading/StringStreamReader.hpp>
#include <shard/parsing/lexical/SourceProvider.hpp>
#include <shard/parsing/analysis/TextLocation.hpp>

#include <sstream>
#include <istream>
#include <string>
#include <codecvt>
#include <locale>

using namespace shard;

StringStreamReader::StringStreamReader(const std::wstring& name, std::wstringstream& source) : SourceTextProvider(), name(name)
{
	stringStream = std::wstringstream(source.str());
	stringStream.imbue(source.getloc());
}

StringStreamReader::StringStreamReader(const std::wstring& name, const std::wstring& source) : SourceTextProvider(), name(name)
{
	stringStream = std::wstringstream(source);
	stringStream.imbue(std::locale("en_US.UTF8"));
}

StringStreamReader::StringStreamReader(const std::wstring& name, const wchar_t* source, size_t count) : SourceTextProvider(), name(name)
{
	std::wstring srcStr = std::wstring(source, count);
	stringStream = std::wstringstream(srcStr);
	stringStream.imbue(std::locale("en_US.UTF8"));
}

StringStreamReader::~StringStreamReader()
{

}

bool StringStreamReader::ReadNext(wchar_t& ch)
{
	wchar_t PeekSymbol = stringStream.peek();
	if (PeekSymbol == WEOF)
		return false;

	stringStream.get(ch);
	if (ch == WEOF)
		return false;

	return true;
}

bool StringStreamReader::PeekNext(wchar_t& ch)
{
	ch = stringStream.peek();
	return ch != WEOF;
}

std::wstring& StringStreamReader::GetName()
{
	return name;
}
