#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/semantic/symbols/TypeSymbol.hpp>
#include <shard/parsing/SyntaxKind.hpp>
#include <shard/semantic/SyntaxSymbol.hpp>

namespace shard
{
    class SHARD_API LeftDenotationSymbol : public SyntaxSymbol
    {
    public:
        const TypeSymbol* ExpectedType;
        
        inline LeftDenotationSymbol(const TypeSymbol* expectedType)
            : SyntaxSymbol(L"", SyntaxKind::Argument), ExpectedType(expectedType) {}

        inline LeftDenotationSymbol(const LeftDenotationSymbol& other) = delete;

        inline virtual ~LeftDenotationSymbol() = default;
    };
}
