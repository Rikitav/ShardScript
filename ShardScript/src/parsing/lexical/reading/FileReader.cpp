#include <shard/parsing/lexical/SourceProvider.hpp>
#include <shard/parsing/lexical/reading/FileReader.hpp>
#include <shard/parsing/analysis/TextLocation.hpp>

#include <fstream>
#include <stdexcept>
#include <string>

using namespace shard;

FileReader::FileReader(const std::wstring& fileName) : SourceTextProvider()
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

bool FileReader::ReadNext(wchar_t& ch)
{
	wchar_t PeekSymbol = InputStream.peek();
	if (PeekSymbol == WEOF)
		return false;

	ch = WEOF;
	InputStream.get(ch);
	if (ch == WEOF)
		return false;

	return true;
}

bool FileReader::PeekNext(wchar_t& ch)
{
	ch = InputStream.peek();
	return ch != WEOF;
}

std::wstring& FileReader::GetName()
{
	return Filename;
}
