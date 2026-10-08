#pragma once
#include <shard/Definitions.hpp>

#include <string_view>

#include <string_intern.h>

namespace gmt
{
	using StringIntern = rs::stringintern::StringIntern;
	using StringReference = rs::stringintern::StringReference;

	SHARD_API StringIntern& global_interner();

	[[nodiscard]] SHARD_API StringReference intern(std::wstring_view text);
	[[nodiscard]] SHARD_API std::wstring_view resolve(StringReference ref);
}

namespace std
{
	template<>
	struct hash<gmt::StringReference>
	{
		std::size_t operator()(const gmt::StringReference& ref) const noexcept
		{
			return static_cast<std::size_t>(ref.Number()) * 2654435761u ^ ref.Index();
		}
	};
}
