#include <shard/lexical/SourceProvider.hpp>
#include <shard/lexical/FileReader.hpp>
#include <shard/parsing/TextLocation.hpp>
#include <gmt/Containers.hpp>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>

using namespace shard;

namespace {

	// wchar_t is UTF-16 on Windows, so a code point outside the BMP has to be
	// written as a surrogate pair there
	inline void append_code_point(gmt::wstring& text, char32_t code)
	{
		if constexpr (sizeof(wchar_t) >= 4)
		{
			text.push_back(static_cast<wchar_t>(code));
			return;
		}

		if (code <= 0xFFFF)
		{
			text.push_back(static_cast<wchar_t>(code));
			return;
		}

		code -= 0x10000;
		text.push_back(static_cast<wchar_t>(0xD800 + (code >> 10)));
		text.push_back(static_cast<wchar_t>(0xDC00 + (code & 0x3FF)));
	}

	// strict: returns false on any malformed or overlong sequence so the caller
	// can fall back to a byte-per-character decode
	bool decode_utf8(const std::string& bytes, gmt::wstring& text)
	{
		text.clear();
		text.reserve(bytes.size()); // Верхняя оценка размера

		std::size_t index = 0;
		if (bytes.size() >= 3
			&& static_cast<unsigned char>(bytes[0]) == 0xEF
			&& static_cast<unsigned char>(bytes[1]) == 0xBB
			&& static_cast<unsigned char>(bytes[2]) == 0xBF)
		{
			index = 3;
		}

		const std::size_t size = bytes.size();
		while (index < size)
		{
			const auto lead = static_cast<unsigned char>(bytes[index]);
			if (lead < 0x80)
			{
				text.push_back(static_cast<wchar_t>(lead));
				++index;
				continue;
			}

			int continuationCount = 0;
			char32_t code = 0;

			if ((lead & 0xE0) == 0xC0)
			{
				continuationCount = 1;
				code = lead & 0x1F;
			}
			else if ((lead & 0xF0) == 0xE0)
			{
				continuationCount = 2;
				code = lead & 0x0F;
			}
			else if ((lead & 0xF8) == 0xF0)
			{
				continuationCount = 3;
				code = lead & 0x07;
			}
			else
			{
				return false;
			}

			if (index + static_cast<std::size_t>(continuationCount) >= size)
				return false;

			for (int offset = 1; offset <= continuationCount; ++offset)
			{
				const auto continuation = static_cast<unsigned char>(bytes[index + offset]);
				if ((continuation & 0xC0) != 0x80)
					return false;

				code = (code << 6) | (continuation & 0x3F);
			}

			index += static_cast<std::size_t>(continuationCount) + 1;

			const bool overlong = (continuationCount == 1 && code < 0x80)
				|| (continuationCount == 2 && code < 0x800)
				|| (continuationCount == 3 && code < 0x10000);

			if (overlong || code > 0x10FFFF || (code >= 0xD800 && code <= 0xDFFF))
				return false;

			append_code_point(text, code);
		}

		return true;
	}

	gmt::wstring load_source_text(const std::filesystem::path& filename)
	{
		// Используем std::filesystem для быстрого получения размера файла
		std::error_code ec;
		const auto file_size = std::filesystem::file_size(filename, ec);
		if (ec)
			throw std::runtime_error("Cannot open file or get its size");

		std::ifstream input(filename, std::ios::binary);
		if (!input)
			throw std::runtime_error("Cannot open file");

		std::string bytes;
		bytes.resize(file_size);

		if (file_size > 0)
		{
			input.read(bytes.data(), static_cast<std::streamsize>(file_size));
			if (!input)
				throw std::runtime_error("Failed to read file content");
		}

		if (bytes.empty())
			return gmt::wstring();

		gmt::wstring text;
		if (decode_utf8(bytes, text))
			return text;

		// Fallback для не-UTF-8 файлов (побайтное копирование)
		text.clear();
		text.reserve(bytes.size());
		for (const char byte : bytes)
		{
			text.push_back(static_cast<wchar_t>(static_cast<unsigned char>(byte)));
		}

		return text;
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
#ifdef __GNUC__
	return gmt::wstring(m_filename.wstring());
#else
	return gmt::wstring(m_filename.c_str());
#endif
}