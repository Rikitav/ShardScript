#include <shard/parsing/reading/SourceReader.h>
#include <shard/parsing/reading/FileReader.h>
#include <shard/parsing/analysis/TextLocation.h>

#include <fstream>
#include <stdexcept>
#include <string>

using namespace std;
using namespace shard::parsing;
using namespace shard::parsing::analysis;

FileReader::FileReader(const wstring& fileName) : SourceReader()
{
	Filename = fileName;

	InputStream = wfstream(fileName, ios::in);
	if (!InputStream)
		throw new runtime_error("Cannot open file");
}

FileReader::~FileReader()
{
	InputStream.close();
}

TextLocation FileReader::GetLocation(wstring& word)
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
