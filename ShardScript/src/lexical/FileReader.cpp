#include <shard/lexical/SourceProvider.hpp>
#include <shard/lexical/FileReader.hpp>
#include <shard/parsing/TextLocation.hpp>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <locale>

using namespace shard;

FileReader::FileReader(const std::string& fileName) : SourceTextProvider()
{
    m_filename = std::filesystem::path(fileName);
    m_inputStream = std::wfstream(m_filename, std::ios::in);
    m_inputStream.imbue(std::locale::classic());

    if (!m_inputStream)
        throw std::runtime_error("Cannot open file");
}

FileReader::FileReader(const std::wstring& fileName) : SourceTextProvider()
{
    m_filename = std::filesystem::path(fileName);
    m_inputStream = std::wfstream(m_filename, std::ios::in);
    m_inputStream.imbue(std::locale::classic());

    if (!m_inputStream)
        throw std::runtime_error("Cannot open file");
}

FileReader::~FileReader()
{
    m_inputStream.close();
}

bool FileReader::read_next(wchar_t& ch)
{
    wchar_t PeekSymbol = m_inputStream.peek();
    if (PeekSymbol == WEOF)
        return false;

    ch = WEOF;
    m_inputStream.get(ch);
    if (ch == WEOF)
        return false;

    return true;
}

bool FileReader::peek_next(wchar_t& ch)
{
    ch = m_inputStream.peek();
    return ch != WEOF;
}

std::wstring FileReader::get_name()
{
    return m_filename.generic_wstring();
}
