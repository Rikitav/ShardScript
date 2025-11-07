#pragma once
#include <shard/syntax/symbols/TypeSymbol.h>
#include <shard/syntax/SyntaxKind.h>
#include <shard/syntax/SyntaxSymbol.h>
#include <shard/syntax/symbols/MethodSymbol.h>
#include <string>

namespace shard::syntax::symbols
{
    class PropertySymbol : public SyntaxSymbol
    {
    public:
        TypeSymbol* ReturnType = nullptr;
        MethodSymbol* GetMethod = nullptr;
        MethodSymbol* SetMethod = nullptr;
        FieldSymbol* BackingField = nullptr;
        bool IsStatic = false;

        inline PropertySymbol(std::wstring name)
            : SyntaxSymbol(name, SyntaxKind::PropertyDeclaration) { }

        inline ~PropertySymbol() override
        {
            if (GetMethod != nullptr)
                delete GetMethod;

            if (SetMethod != nullptr)
                delete SetMethod;

            if (BackingField != nullptr)
                delete BackingField;
        }

        PropertySymbol* GenerateBackingField();
    };
}

