#pragma once
#include <shard/syntax/symbols/TypeSymbol.h>

namespace shard::framework
{
	class BooleanPrimitive
	{
	public:
		static void Reflect(shard::syntax::symbols::TypeSymbol*);
	};
}
