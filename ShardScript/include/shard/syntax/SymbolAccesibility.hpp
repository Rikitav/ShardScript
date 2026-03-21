#pragma once
#include <shard/ShardScriptAPI.hpp>

#define ACS_PUBLIC shard::SymbolAccesibility::Public
#define ACS_PRIVATE shard::SymbolAccesibility::Private

namespace shard
{
	enum class SymbolAccesibility
	{
		Private,
		Public,
		Protected
	};
}
