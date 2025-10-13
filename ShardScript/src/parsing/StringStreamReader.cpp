#include <shard/parsing/StringStreamReader.h>

using namespace shard::parsing;

StringStreamReader::StringStreamReader(const string& source) : SourceReader()
{
	stringStream = istringstream(source);
}

TextLocation StringStreamReader::GetLocation(string& word)
{
	return TextLocation("<STRING>", Line, Offset, static_cast<int>(word.length()));
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
