#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/syntax/SyntaxKind.hpp>
#include <shard/syntax/SyntaxSymbol.hpp>

#include <shard/syntax/symbols/TypeSymbol.hpp>
#include <shard/syntax/symbols/AccessorSymbol.hpp>
#include <shard/syntax/symbols/FieldSymbol.hpp>

#include <shard/syntax/nodes/ExpressionSyntax.hpp>

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

