#pragma once
#include <shard/ShardScriptAPI.h>

#include <shard/parsing/reading/SourceReader.h>
#include <shard/parsing/analysis/TextLocation.h>

#include <fstream>
#include <string>

namespace shard::parsing
{
	class SHARD_API FileReader : public SourceReader
	{
		std::wstring Filename;
		std::wfstream InputStream;

	public:
		FileReader(const std::wstring& fileName);
		~FileReader() override;

		shard::parsing::analysis::TextLocation GetLocation(std::wstring& word) override;

	protected:
		bool ReadNext() override;
		bool PeekNext() override;
	};
}