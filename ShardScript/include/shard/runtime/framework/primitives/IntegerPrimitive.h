#pragma once
#include <shard/syntax/symbols/TypeSymbol.h>

namespace shard::parsing::semantic
{
	class IntegerPrimitive
	{
	public:
		static void Reflect(shard::syntax::symbols::TypeSymbol*);
	};
}
