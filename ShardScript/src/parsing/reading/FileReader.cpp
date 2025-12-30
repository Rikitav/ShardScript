#include <shard/parsing/reading/SourceReader.h>
#include <shard/parsing/reading/FileReader.h>
#include <shard/parsing/analysis/TextLocation.h>

#include <fstream>
#include <stdexcept>
#include <string>

using namespace shard;

FileReader::FileReader(const std::wstring& fileName) : SourceReader()
{
	Filename = fileName;
	InputStream = std::wfstream(fileName, std::ios::in);
	InputStream.imbue(std::locale("en_US.UTF8"));

	if (!InputStream)
		throw std::runtime_error("Cannot open file");
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
	PeekSymbol = InputStream.peek();
	if (PeekSymbol == WEOF)
		return false;

	InputStream.get(Symbol);
	if (Symbol == WEOF)
		return false;

	Offset++;
	return true;
}

bool FileReader::PeekNext()
{
	PeekSymbol = InputStream.peek();
	return PeekSymbol != WEOF;
}
