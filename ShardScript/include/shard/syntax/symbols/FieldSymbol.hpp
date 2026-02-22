#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/syntax/nodes/ExpressionSyntax.hpp>
#include <shard/syntax/symbols/TypeSymbol.hpp>

#include <shard/syntax/SyntaxKind.hpp>
#include <shard/syntax/SyntaxSymbol.hpp>

#include <string>

namespace shard
{
    class SHARD_API FieldSymbol : public SyntaxSymbol
    {
    public:
        size_t MemoryBytesOffset = 0;
        TypeSymbol* ReturnType = nullptr;
        shard::ExpressionSyntax* DefaultValueExpression = nullptr;
        bool IsStatic = false;

        inline FieldSymbol(std::wstring name)
            : SyntaxSymbol(name, SyntaxKind::FieldDeclaration) { }

        inline FieldSymbol(const FieldSymbol& other) = delete;

        inline virtual ~FieldSymbol()
        {
            if (DefaultValueExpression != nullptr)
                delete DefaultValueExpression;
        }
    };
}