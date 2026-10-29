#include <shard/lexical/SourceProvider.hpp>
#include <shard/lexical/FileReader.hpp>
#include <shard/parsing/TextLocation.hpp>

#include <gmt/Containers.hpp>

#include <codecvt>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <locale>
#include <stdexcept>
#include <string>
#include <vector>

using namespace shard;

namespace
{
	// Bulk equivalent of the old std::wfstream(..., classic) character path:
	// same codecvt facet, one conversion instead of one streambuf peek/get per
	// character. On a conversion error the text stops where the stream would
	// have reported EOF.
	gmt::wstring load_source_text(const std::filesystem::path& filename)
	{
		std::ifstream input(filename, std::ios::binary);
		if (!input)
			throw std::runtime_error("Cannot open file");

		const std::string bytes(
			(std::istreambuf_iterator<char>(input)),
			std::istreambuf_iterator<char>());

		if (bytes.empty())
			return gmt::wstring();

		const std::locale classic = std::locale::classic();
		const std::codecvt<wchar_t, char, std::mbstate_t>& converter =
			std::use_facet<std::codecvt<wchar_t, char, std::mbstate_t>>(classic);

		std::vector<wchar_t> buffer(bytes.size() + 1);
		std::mbstate_t state{};

		const char* from = bytes.data();
		const char* fromEnd = from + bytes.size();
		wchar_t* to = buffer.data();
		wchar_t* toEnd = to + buffer.size();

		converter.in(state, from, fromEnd, from, to, toEnd, to);
		return gmt::wstring(buffer.data(), static_cast<std::size_t>(to - buffer.data()));
	}
}

FileReader::FileReader(const gmt::string& fileName) :
	SourceTextProvider(),
	m_filename(fileName),
	m_text(load_source_text(m_filename))
{ }

FileReader::FileReader(const gmt::wstring& fileName) :
	SourceTextProvider(),
	m_filename(fileName),
	m_text(load_source_text(m_filename))
{ }

FileReader::~FileReader() = default;

bool FileReader::read_next(wchar_t& ch)
{
	if (m_index >= m_text.size())
		return false;

	ch = m_text[m_index++];
	return true;
}

bool FileReader::peek_next(wchar_t& ch)
{
	if (m_index >= m_text.size())
		return false;

	ch = m_text[m_index];
	return true;
}

gmt::wstring FileReader::get_name()
{
	return gmt::wstring(m_filename.c_str());
}
