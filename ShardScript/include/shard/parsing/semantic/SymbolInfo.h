#pragma once
#include <shard/ShardScriptAPI.h>
#include <shard/syntax/SyntaxSymbol.h>

namespace shard
{
    struct SHARD_API SymbolInfo
    {
    public:
        shard::SyntaxSymbol* Symbol;

        inline SymbolInfo(shard::SyntaxSymbol* symbol)
            : Symbol(symbol) { }
    };
}
