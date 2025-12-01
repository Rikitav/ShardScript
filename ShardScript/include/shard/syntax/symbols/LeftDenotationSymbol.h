#pragma once
#include <shard/ShardScriptAPI.h>

#include <shard/syntax/symbols/TypeSymbol.h>
#include <shard/syntax/SyntaxKind.h>
#include <shard/syntax/SyntaxSymbol.h>

namespace shard::syntax::symbols
{
    class SHARD_API LeftDenotationSymbol : public SyntaxSymbol
    {
    public:
        const TypeSymbol* ExpectedType;
        
        inline LeftDenotationSymbol(const TypeSymbol* expectedType)
            : SyntaxSymbol(L"", SyntaxKind::Argument), ExpectedType(expectedType) {}

        inline LeftDenotationSymbol(const LeftDenotationSymbol& other) = delete;

        inline ~LeftDenotationSymbol()
        {

        }
    };
}
