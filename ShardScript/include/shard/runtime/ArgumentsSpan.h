#pragma once
#include <shard/ShardScriptAPI.h>

#include <shard/runtime/ObjectInstance.h>
#include <shard/syntax/symbols/MethodSymbol.h>

#include <vector>
#include <string>

namespace shard
{
	class SHARD_API ArgumentsSpan
	{
		const MethodSymbol* Method;
		const std::vector<ObjectInstance*> Span;

	public:
		inline ArgumentsSpan(const MethodSymbol* method, std::vector<ObjectInstance*> span)
			: Method(method), Span(span) { }

		inline ArgumentsSpan(const ArgumentsSpan& other) = delete;

		~ArgumentsSpan();

		ObjectInstance* operator[](int index) const;
		ObjectInstance* Find(const std::wstring& name) const;
	};
}