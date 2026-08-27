#include <shard/lexical/SourceProvider.hpp>
#include <shard/lexical/FileReader.hpp>
#include <shard/analysis/TextLocation.hpp>

#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <filesystem>

using namespace shard;

#ifndef __EMSCRIPTEN__
FileReader::FileReader(const std::string& fileName) : SourceTextProvider()
{
    std::filesystem::path path(fileName);
    Filename = path.wstring();
    InputStream = std::wfstream(path, std::ios::in);
    InputStream.imbue(std::locale::classic());

    if (!InputStream)
        throw std::runtime_error("Cannot open file");
}

FileReader::FileReader(const std::wstring& fileName) : SourceTextProvider()
{
    std::filesystem::path path(fileName);
    Filename = path.wstring();
    InputStream = std::wfstream(path, std::ios::in);
    InputStream.imbue(std::locale::classic());

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

#endif // __EMSCRIPTEN__
