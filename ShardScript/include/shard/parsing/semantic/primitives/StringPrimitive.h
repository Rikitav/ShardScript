#pragma once
#include <shard/syntax/symbols/TypeSymbol.h>

namespace shard::parsing::semantic
{
	class StringPrimitive
	{
	public:
		static void Reflect(shard::syntax::symbols::TypeSymbol*);
	};
}
