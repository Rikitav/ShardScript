#pragma once
#include <shard/syntax/SyntaxKind.h>
#include <shard/syntax/SyntaxSymbol.h>
#include <shard/syntax/symbols/MethodSymbol.h>
#include <string>

namespace shard::syntax::symbols
{
    class AccessorSymbol : public SyntaxSymbol
    {
    public:
        MethodSymbol* Method = nullptr;

        inline AccessorSymbol(std::wstring name)
            : SyntaxSymbol(name, SyntaxKind::AccessorDeclaration) { }
    };
}