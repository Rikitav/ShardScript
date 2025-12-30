#pragma once
#include <shard/ShardScriptAPI.h>

#include <shard/syntax/nodes/ExpressionSyntax.h>
#include <shard/syntax/symbols/TypeSymbol.h>

#include <shard/syntax/SyntaxKind.h>
#include <shard/syntax/SyntaxSymbol.h>

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