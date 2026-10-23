#pragma once
#include <shard/Definitions.hpp>
#include <shard/lexical/SourceTextProvider.hpp>
#include <shard/parsing/TextLocation.hpp>

#include <gmt/Containers.hpp>

#include <sstream>
#include <string>

namespace shard
{
	class SHARD_API StringStreamReader : public SourceTextProvider
	{
		gmt::wstring m_name;
		std::wstringstream m_stringStream;

	public:
		StringStreamReader(const gmt::wstring& name, std::wstringstream& source);
		StringStreamReader(const gmt::wstring& name, const gmt::wstring& source);
		StringStreamReader(const gmt::wstring& name, string_t source, std::size_t size);
		virtual ~StringStreamReader();

		bool read_next(wchar_t& ch) override;
		bool peek_next(wchar_t& ch) override;
		gmt::wstring get_name() override;
	};
}
