#pragma once
#include <shard/ShardScriptAPI.hpp>

namespace shard
{
	class SymbolFactory;

	void RegisterPrimitiveOperators(SymbolFactory& factory);
	void RegisterGlobalMethods(SymbolFactory& factory);
}
