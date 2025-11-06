#pragma once
#include <shard/syntax/symbols/TypeSymbol.h>

namespace shard::parsing::semantic
{
	class ArrayPrimitive
	{
	public:
		static void Reflect(shard::syntax::symbols::TypeSymbol*);
	};
}
