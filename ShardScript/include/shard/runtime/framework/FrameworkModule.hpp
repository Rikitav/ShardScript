#pragma once
#include <shard/ShardScriptAPI.hpp>
#include <shard/parsing/lexical/SourceProvider.hpp>
#include <shard/syntax/symbols/MethodSymbol.hpp>
#include <shard/syntax/symbols/AccessorSymbol.hpp>
#include <shard/syntax/symbols/ConstructorSymbol.hpp>

#include <string>

namespace shard
{
	class SHARD_API FrameworkModule
	{
	public:
		virtual shard::SourceProvider* GetSource() = 0;
		virtual bool BindConstructor(shard::ConstructorSymbol* symbol) = 0;
		virtual bool BindMethod(shard::MethodSymbol* symbol) = 0;
		virtual bool BindAccessor(shard::AccessorSymbol* symbol) = 0;
	};
}