#pragma once
#include <shard/Definitions.hpp>

#include <string>
#include <string_view>

namespace gmt
{
	SHARD_API std::string wide_to_utf8(std::wstring_view wstr);
	SHARD_API std::string wide_to_utf8(const wchar_t* wstr);

	SHARD_API std::wstring utf8_to_wide(std::string_view narrow);
	SHARD_API std::wstring utf8_to_wide(const char* narrow);

	SHARD_API std::string wide_to_ansi(std::wstring_view wstr);
	SHARD_API std::string wide_to_ansi(const wchar_t* wstr);

	SHARD_API std::wstring ansi_to_wide(std::string_view narrow);
	SHARD_API std::wstring ansi_to_wide(const char* narrow);

	SHARD_API std::wstring utf16_to_wide(const void* ptr);

	SHARD_API std::string wide_to_narrow(std::wstring_view wstr);
	SHARD_API std::string wide_to_narrow(const wchar_t* wstr);
}
