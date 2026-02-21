#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/syntax/symbols/TypeSymbol.hpp>
#include <shard/syntax/SyntaxKind.hpp>
#include <shard/syntax/SyntaxSymbol.hpp>

namespace shard
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
