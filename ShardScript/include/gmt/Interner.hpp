#pragma once
#include <shard/Definitions.hpp>

#include <string_view>

#include <string_intern.h>

namespace gmt
{
	SHARD_API rs::stringintern::StringIntern& global_interner();

	[[nodiscard]] SHARD_API rs::stringintern::StringReference intern(std::wstring_view text);
	[[nodiscard]] SHARD_API std::wstring_view resolve(rs::stringintern::StringReference ref);
}
