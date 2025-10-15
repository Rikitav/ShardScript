#include <shard/parsing/SourceReader.h>
#include <shard/parsing/FileReader.h>
#include <shard/syntax/analysis/TextLocation.h>

#include <fstream>
#include <stdexcept>
#include <string>

using namespace shard::parsing;

FileReader::FileReader(const string& fileName) : SourceReader()
{
	Filename = fileName;

	InputStream = fstream(fileName, ios::in);
	if (!InputStream)
		throw new runtime_error("Cannot open file");
}

FileReader::~FileReader()
{
	InputStream.close();
}

TextLocation FileReader::GetLocation(string& word)
{
	return TextLocation(Filename, Line, Offset, static_cast<int>(word.length()));
}

bool FileReader::ReadNext()
{
	if (!InputStream.get(Symbol))
		return false;

	Offset++;
	return true;
}

bool FileReader::PeekNext()
{
	PeekSymbol = InputStream.peek();
	return PeekSymbol != -1;
}
