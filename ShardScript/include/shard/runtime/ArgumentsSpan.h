#pragma once
#include <shard/ShardScriptAPI.h>

#include <shard/runtime/ObjectInstance.h>
#include <shard/syntax/symbols/MethodSymbol.h>

#include <unordered_map>
#include <string>

namespace shard
{
	class SHARD_API ArgumentsSpan
	{
		const MethodSymbol* Method;
		const ObjectInstance* Start;
		const size_t Size;

	public:
		inline ArgumentsSpan(const MethodSymbol* method, const ObjectInstance* start, const size_t size)
			: Method(method), Start(start), Size(size) { }

		/*
		inline ArgumentsSpan(const MethodSymbol* method, std::initializer_list<ObjectInstance*> args) : Method(method), Start(nullptr), Size(0)
		{
			
		}
		*/

		inline ArgumentsSpan(const ArgumentsSpan& other) = delete;

		~ArgumentsSpan();

		const ObjectInstance* operator[](int index);
		const ObjectInstance* Find(const std::wstring& name);
	};
}