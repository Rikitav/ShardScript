#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <string>

namespace shard
{
	class SHARD_API SourceTextProvider
	{
	public:
		virtual bool ReadNext(wchar_t& ch) = 0;
		virtual bool PeekNext(wchar_t& ch) = 0;
		virtual std::wstring& GetName() = 0;
	};
}