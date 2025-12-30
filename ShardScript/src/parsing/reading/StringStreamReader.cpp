#include <shard/parsing/reading/StringStreamReader.h>
#include <shard/parsing/reading/SourceReader.h>
#include <shard/parsing/analysis/TextLocation.h>

#include <sstream>
#include <istream>
#include <string>
#include <codecvt>
#include <locale>

using namespace shard;

StringStreamReader::StringStreamReader(const std::wstring& name, std::wstringstream& source) : SourceReader(), name(name)
{
	stringStream = std::wstringstream(source.str());
	stringStream.imbue(source.getloc());
}

StringStreamReader::StringStreamReader(const std::wstring& name, const std::wstring& source) : SourceReader(), name(name)
{
	stringStream = std::wstringstream(source);
	stringStream.imbue(std::locale("en_US.UTF8"));
}

StringStreamReader::StringStreamReader(const std::wstring& name, const wchar_t* source, size_t count) : SourceReader(), name(name)
{
	std::wstring srcStr = std::wstring(source, count);
	stringStream = std::wstringstream(srcStr);
	stringStream.imbue(std::locale("en_US.UTF8"));
}

TextLocation StringStreamReader::GetLocation(std::wstring& word)
{
	return TextLocation(name, Line, Offset, static_cast<int>(word.length()));
}

bool StringStreamReader::ReadNext()
{
	PeekSymbol = stringStream.peek();
	if (PeekSymbol == WEOF)
		return false;

	stringStream.get(Symbol);
	if (Symbol == WEOF)
		return false;

	Offset++;
	return true;
}

bool StringStreamReader::PeekNext()
{
	PeekSymbol = stringStream.peek();
	return PeekSymbol != WEOF;
}
