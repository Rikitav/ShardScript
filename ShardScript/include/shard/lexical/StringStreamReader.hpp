#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/lexical/SourceTextProvider.hpp>
#include <shard/analysis/TextLocation.hpp>

#include <sstream>
#include <string>

namespace shard
{
	class SHARD_API StringStreamReader : public SourceTextProvider
	{
		std::wstring name;
		std::wstringstream stringStream;

	public:
		StringStreamReader(const std::wstring& name, std::wstringstream& source);
		StringStreamReader(const std::wstring& name, const std::wstring& source);
		StringStreamReader(const std::wstring& name, const wchar_t* source, std::size_t size);
		virtual ~StringStreamReader();

		bool ReadNext(wchar_t& ch) override;
		bool PeekNext(wchar_t& ch) override;
		std::wstring& GetName() override;
	};
}
