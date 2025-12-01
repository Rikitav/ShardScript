#pragma once
#include <shard/ShardScriptAPI.h>
#include <shard/syntax/SyntaxSymbol.h>

namespace shard::parsing::semantic
{
    struct SHARD_API SymbolInfo
    {
    public:
        shard::syntax::SyntaxSymbol* Symbol;

        inline SymbolInfo(shard::syntax::SyntaxSymbol* symbol)
            : Symbol(symbol) { }
    };
}
