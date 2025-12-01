#pragma once
#include <shard/ShardScriptAPI.h>
#include <shard/syntax/symbols/MethodSymbol.h>

#include <string>

namespace shard::framework
{
	class SHARD_API FrameworkModule
	{
	public:
		virtual std::wstring& GetSourceCode() = 0;
		virtual bool BindMethod(shard::syntax::symbols::MethodSymbol* symbol) = 0;
	};
}