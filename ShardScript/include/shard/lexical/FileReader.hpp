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
		std::wfstream m_inputStream;

	public:
		FileReader(const gmt::string& fileName);
		FileReader(const gmt::wstring& fileName);
		virtual ~FileReader();

		bool read_next(wchar_t& ch) override;
		bool peek_next(wchar_t& ch) override;
		gmt::wstring get_name() override;
	};
}
