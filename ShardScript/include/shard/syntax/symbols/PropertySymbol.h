#pragma once
#include <shard/syntax/SyntaxKind.h>
#include <shard/syntax/SyntaxSymbol.h>

#include <shard/syntax/symbols/TypeSymbol.h>
#include <shard/syntax/symbols/AccessorSymbol.h>
#include <shard/syntax/symbols/FieldSymbol.h>
#include <string>

namespace shard::syntax::symbols
{
    class PropertySymbol : public SyntaxSymbol
    {
    public:
        TypeSymbol* ReturnType = nullptr;
        AccessorSymbol* Getter = nullptr;
        AccessorSymbol* Setter = nullptr;
        FieldSymbol* BackingField = nullptr;
        bool IsStatic = false;

        inline PropertySymbol(std::wstring name)
            : SyntaxSymbol(name, SyntaxKind::PropertyDeclaration) { }

        inline ~PropertySymbol() override
        {
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

