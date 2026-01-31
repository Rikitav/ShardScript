#pragma once
#include <shard/ShardScriptAPI.h>

#include <shard/syntax/SyntaxKind.h>
#include <shard/syntax/SyntaxSymbol.h>

#include <shard/syntax/symbols/TypeSymbol.h>
#include <shard/syntax/symbols/AccessorSymbol.h>
#include <shard/syntax/symbols/FieldSymbol.h>

#include <shard/syntax/nodes/ExpressionSyntax.h>

#include <string>

namespace shard
{
    class SHARD_API PropertySymbol : public SyntaxSymbol
    {
    public:
        shard::ExpressionSyntax* DefaultValueExpression = nullptr;
        TypeSymbol* ReturnType = nullptr;
        AccessorSymbol* Getter = nullptr;
        AccessorSymbol* Setter = nullptr;
        FieldSymbol* BackingField = nullptr;
        bool IsStatic = false;

        inline PropertySymbol(const std::wstring& name)
            : SyntaxSymbol(name, SyntaxKind::PropertyDeclaration) { }

        inline PropertySymbol(const std::wstring& name, const SyntaxKind kind)
            : SyntaxSymbol(name, kind) { }

        inline PropertySymbol(const PropertySymbol& other) = delete;

        inline virtual ~PropertySymbol() override
        {
            if (DefaultValueExpression != nullptr)
                delete DefaultValueExpression;

            if (Getter != nullptr)
                delete Getter;

            if (Setter != nullptr)
                delete Setter;

            if (BackingField != nullptr)
                delete BackingField;
        }

        PropertySymbol* GenerateBackingField();
    };
}

