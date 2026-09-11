#pragma once
#include <shard/Definitions.hpp>

#include <string>

namespace shard
{
	class SHARD_API SourceTextProvider
	{
	public:
		virtual ~SourceTextProvider() = default;

		virtual bool read_next(wchar_t& ch) = 0;
		virtual bool peek_next(wchar_t& ch) = 0;
		virtual std::wstring get_name() = 0;
	};
}