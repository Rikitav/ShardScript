#pragma once
#include <shard/syntax/symbols/TypeSymbol.h>
#include <shard/syntax/SyntaxKind.h>
#include <shard/syntax/SyntaxSymbol.h>

namespace shard::syntax::symbols
{
    class LeftDenotationSymbol : public SyntaxSymbol
    {
    public:
        const TypeSymbol* ExpectedType;
        
        inline LeftDenotationSymbol(const TypeSymbol* expectedType)
            : SyntaxSymbol(L"", SyntaxKind::Argument), ExpectedType(expectedType) {}
        
        inline ~LeftDenotationSymbol() = default;
    };
}
