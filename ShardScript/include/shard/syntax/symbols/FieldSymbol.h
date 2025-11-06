#pragma once
#include <shard/syntax/nodes/ExpressionSyntax.h>
#include <shard/syntax/symbols/TypeSymbol.h>
#include <shard/syntax/SyntaxKind.h>
#include <shard/syntax/SyntaxSymbol.h>
#include <string>

namespace shard::syntax::symbols
{
    class FieldSymbol : public SyntaxSymbol
    {
    public:
        size_t MemoryBytesOffset = 0;
        TypeSymbol* ReturnType = nullptr;
        shard::syntax::nodes::ExpressionSyntax* DefaultValueExpression = nullptr;
        bool IsStatic = false;

        inline FieldSymbol(std::wstring name)
            : SyntaxSymbol(name, SyntaxKind::FieldDeclaration) { }
    };
}