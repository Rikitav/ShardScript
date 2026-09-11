#pragma once
#include <shard/Definitions.hpp>
#include <shard/lexical/SourceTextProvider.hpp>
#include <shard/parsing/TextLocation.hpp>

#include <sstream>
#include <string>

namespace shard
{
	class SHARD_API StringStreamReader : public SourceTextProvider
	{
		std::wstring m_name;
		std::wstringstream m_stringStream;

	public:
		StringStreamReader(const std::wstring& name, std::wstringstream& source);
		StringStreamReader(const std::wstring& name, const std::wstring& source);
		StringStreamReader(const std::wstring& name, string_t source, std::size_t size);
		virtual ~StringStreamReader() = default;

		bool read_next(wchar_t& ch) override;
		bool peek_next(wchar_t& ch) override;
		std::wstring get_name() override;
	};
}
