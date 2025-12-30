#pragma once
#include <shard/syntax/symbols/TypeSymbol.h>

namespace shard
{
	class ArrayPrimitive
	{
	public:
		static void Reflect(shard::TypeSymbol*);
	};

	class BooleanPrimitive
	{
	public:
		static void Reflect(shard::TypeSymbol*);
	};

	class CharPrimitive
	{
	public:
		static void Reflect(shard::TypeSymbol*);
	};

	class IntegerPrimitive
	{
	public:
		static void Reflect(shard::TypeSymbol*);
	};

	class DoublePrimitive
	{
	public:
		static void Reflect(shard::TypeSymbol*);
	};

	class StringPrimitive
	{
	public:
		static void Reflect(shard::TypeSymbol*);
	};
}
