#pragma once
#include <shard/syntax/symbols/TypeSymbol.h>

namespace shard::framework
{
	class ArrayPrimitive
	{
	public:
		static void Reflect(shard::syntax::symbols::TypeSymbol*);
	};

	class BooleanPrimitive
	{
	public:
		static void Reflect(shard::syntax::symbols::TypeSymbol*);
	};

	class CharPrimitive
	{
	public:
		static void Reflect(shard::syntax::symbols::TypeSymbol*);
	};

	class IntegerPrimitive
	{
	public:
		static void Reflect(shard::syntax::symbols::TypeSymbol*);
	};

	class StringPrimitive
	{
	public:
		static void Reflect(shard::syntax::symbols::TypeSymbol*);
	};
}
