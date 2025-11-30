#pragma once
#include <shard/syntax/symbols/TypeSymbol.h>
#include <shard/syntax/nodes/ExpressionSyntax.h>
#include <shard/syntax/SyntaxKind.h>
#include <shard/syntax/SyntaxSymbol.h>
#include <string>

namespace shard::syntax::symbols
{
	class ParameterSymbol : public SyntaxSymbol
	{
    public:
        TypeSymbol* Type = nullptr;
        bool IsOptional = false;
        shard::syntax::nodes::ExpressionSyntax* DefaultValueExpression = nullptr;

        inline ParameterSymbol(std::wstring name) : SyntaxSymbol(name, SyntaxKind::Parameter)
        {
            Accesibility = SymbolAccesibility::Public;
        }

        inline ~ParameterSymbol()
        {
            if (DefaultValueExpression != nullptr)
            {
                delete DefaultValueExpression;
                DefaultValueExpression = nullptr;
            }
        }
	};
}
