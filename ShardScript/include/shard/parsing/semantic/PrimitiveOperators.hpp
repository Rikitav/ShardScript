#pragma once
#include <shard/ShardScriptAPI.hpp>

namespace shard
{
	class SymbolFactory;

	SHARD_API void RegisterPrimitiveOperators(SymbolFactory& factory);
}
