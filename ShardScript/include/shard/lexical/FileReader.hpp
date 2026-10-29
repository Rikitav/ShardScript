#pragma once
#include <shard/Definitions.hpp>
#include <shard/lexical/SourceTextProvider.hpp>
#include <shard/parsing/TextLocation.hpp>

#include <gmt/Containers.hpp>

#include <fstream>
#include <filesystem>
#include <string>

namespace shard
{
	class SHARD_API FileReader : public SourceTextProvider
	{
		std::filesystem::path m_filename;
		// the whole source is decoded once; the previous wfstream performed a
		// streambuf peek/get for every character the lexer consumed
		gmt::wstring m_text;
		std::size_t m_index = 0;

	public:
		FileReader(const gmt::string& fileName);
		FileReader(const gmt::wstring& fileName);
		virtual ~FileReader();

		bool read_next(wchar_t& ch) override;
		bool peek_next(wchar_t& ch) override;
		gmt::wstring get_name() override;
	};
}
