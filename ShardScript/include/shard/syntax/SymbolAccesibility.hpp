#pragma once
#include <shard/ShardScriptAPI.hpp>

namespace shard
{
	enum class SymbolAccesibility
	{
		Private,
		Public
	};

	constexpr shard::SymbolAccesibility ACS_PUBLIC = shard::SymbolAccesibility::Public;
	constexpr shard::SymbolAccesibility ACS_PRIVATE = shard::SymbolAccesibility::Private;
}
