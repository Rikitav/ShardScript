#pragma once
#include <shard/ShardScriptAPI.hpp>
#include <shard/syntax/SyntaxSymbol.hpp>

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
