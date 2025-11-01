#pragma once
#include <shard/syntax/SyntaxKind.h>
#include <shard/syntax/SyntaxSymbol.h>
#include <shard/syntax/symbols/TypeSymbol.h>
#include <shard/syntax/symbols/ParameterSymbol.h>
#include <shard/syntax/nodes/StatementsBlockSyntax.h>
#include <string>
#include <vector>

namespace shard::syntax::symbols
{
    class MethodSymbol : public SyntaxSymbol
    {
    public:
        TypeSymbol* ReturnType = nullptr;
        MethodSymbol* OverriddenMethod = nullptr;
        std::vector<ParameterSymbol*> Parameters;
        shard::syntax::nodes::StatementsBlockSyntax* Body = nullptr;

        bool IsVirtual = false;
        bool IsOverride = false;
        bool IsStatic = false;

        inline MethodSymbol(std::wstring& name, shard::syntax::nodes::StatementsBlockSyntax* body)
            : SyntaxSymbol(name, SyntaxKind::MethodDeclaration), Body(body) { }
    };
}
