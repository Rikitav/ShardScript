#pragma once
#include <shard/ShardScriptAPI.h>

#include <shard/parsing/reading/SourceReader.h>
#include <shard/parsing/analysis/TextLocation.h>

#include <sstream>
#include <string>

namespace shard
{
	class SHARD_API StringStreamReader : public SourceReader
	{
		std::wstring name;
		std::wstringstream stringStream;

	public:
		StringStreamReader(const std::wstring& name, std::wstringstream& source);
		StringStreamReader(const std::wstring& name, const std::wstring& source);
		StringStreamReader(const std::wstring& name, const wchar_t* source, size_t size);

	protected:
		shard::TextLocation GetLocation(std::wstring& word) override;
		bool ReadNext() override;
		bool PeekNext() override;
	};
}
