#pragma once
#include <shard/syntax/SyntaxSymbol.h>

namespace shard::parsing::semantic
{
    struct SymbolInfo
    {
    public:
        shard::syntax::SyntaxSymbol* Symbol;

        inline SymbolInfo(shard::syntax::SyntaxSymbol* symbol)
            : Symbol(symbol) {
        }
    };
}
