#pragma once
#include <shard/ShardScriptAPI.h>

#include <shard/parsing/reading/SourceReader.h>
#include <shard/parsing/analysis/TextLocation.h>

#include <sstream>
#include <string>

namespace shard::parsing
{
	class SHARD_API StringStreamReader : public SourceReader
	{
		std::wistringstream stringStream;

	public:
		StringStreamReader(const std::wstring& source);

	protected:
		shard::parsing::analysis::TextLocation GetLocation(std::wstring& word) override;
		bool ReadNext() override;
		bool PeekNext() override;
	};
}
