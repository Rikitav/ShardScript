#pragma once
#include <shard/ShardScriptAPI.h>
#include <shard/parsing/reading/SourceReader.h>
#include <shard/syntax/symbols/MethodSymbol.h>
#include <shard/syntax/symbols/AccessorSymbol.h>
#include <shard/syntax/symbols/ConstructorSymbol.h>

#include <string>

namespace shard::framework
{
	class SHARD_API FrameworkModule
	{
	public:
		virtual shard::parsing::SourceReader* GetSource() = 0;
		virtual bool BindConstructor(shard::syntax::symbols::ConstructorSymbol* symbol) = 0;
		virtual bool BindMethod(shard::syntax::symbols::MethodSymbol* symbol) = 0;
		virtual bool BindAccessor(shard::syntax::symbols::AccessorSymbol* symbol) = 0;
	};
}