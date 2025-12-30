#pragma once
#include <shard/ShardScriptAPI.h>
#include <shard/parsing/reading/SourceReader.h>
#include <shard/syntax/symbols/MethodSymbol.h>
#include <shard/syntax/symbols/AccessorSymbol.h>
#include <shard/syntax/symbols/ConstructorSymbol.h>

#include <string>

namespace shard
{
	class SHARD_API FrameworkModule
	{
	public:
		virtual shard::SourceReader* GetSource() = 0;
		virtual bool BindConstructor(shard::ConstructorSymbol* symbol) = 0;
		virtual bool BindMethod(shard::MethodSymbol* symbol) = 0;
		virtual bool BindAccessor(shard::AccessorSymbol* symbol) = 0;
	};
}