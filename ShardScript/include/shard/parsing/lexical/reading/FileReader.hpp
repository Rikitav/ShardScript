#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/parsing/lexical/reading/SourceTextProvider.hpp>
#include <shard/parsing/analysis/TextLocation.hpp>

#include <fstream>
#include <string>

namespace shard
{
	class SHARD_API FileReader : public SourceTextProvider
	{
		std::wstring Filename;
		std::wfstream InputStream;

	public:
		FileReader(const std::wstring& fileName);
		virtual ~FileReader();

		bool ReadNext(wchar_t& ch) override;
		bool PeekNext(wchar_t& ch) override;
		std::wstring& GetName() override;
	};
}