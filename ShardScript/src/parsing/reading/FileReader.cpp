#include <shard/parsing/reading/SourceReader.h>
#include <shard/parsing/reading/FileReader.h>
#include <shard/parsing/analysis/TextLocation.h>

#include <fstream>
#include <stdexcept>
#include <string>

using namespace shard::parsing;
using namespace shard::parsing::analysis;

FileReader::FileReader(const std::wstring& fileName) : SourceReader()
{
	Filename = fileName;

	InputStream = std::wfstream(fileName, std::ios::in);
	if (!InputStream)
		throw new std::runtime_error("Cannot open file");
}

FileReader::~FileReader()
{
	InputStream.close();
}

TextLocation FileReader::GetLocation(std::wstring& word)
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
