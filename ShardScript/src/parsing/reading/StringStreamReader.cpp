#include <shard/parsing/reading/StringStreamReader.h>
#include <shard/parsing/reading/SourceReader.h>
#include <shard/parsing/analysis/TextLocation.h>

#include <sstream>
#include <istream>
#include <string>

using namespace shard::parsing;
using namespace shard::parsing::analysis;

StringStreamReader::StringStreamReader(const std::wstring& source) : SourceReader()
{
	stringStream = std::wistringstream(source);
}

TextLocation StringStreamReader::GetLocation(std::wstring& word)
{
	return TextLocation(L"<STRING>", Line, Offset, static_cast<int>(word.length()));
}

bool StringStreamReader::ReadNext()
{
	if (!stringStream.get(Symbol))
		return false;

	Offset++;
	return true;
}

bool StringStreamReader::PeekNext()
{
	PeekSymbol = stringStream.peek();
	return PeekSymbol != -1;
}
