#pragma once
#include <shard/ShardScriptAPI.h>

#include <shard/syntax/symbols/TypeSymbol.h>
#include <shard/syntax/nodes/ExpressionSyntax.h>

#include <shard/syntax/SyntaxKind.h>
#include <shard/syntax/SyntaxSymbol.h>

#include <string>

namespace shard::syntax::symbols
{
	class SHARD_API ParameterSymbol : public SyntaxSymbol
	{
    public:
        TypeSymbol* Type = nullptr;
        bool IsOptional = false;
        shard::syntax::nodes::ExpressionSyntax* DefaultValueExpression = nullptr;

        inline ParameterSymbol(std::wstring name) : SyntaxSymbol(name, SyntaxKind::Parameter)
        {
            Accesibility = SymbolAccesibility::Public;
        }

        inline ParameterSymbol(const ParameterSymbol& other) = delete;

        inline virtual ~ParameterSymbol()
        {
            if (DefaultValueExpression != nullptr)
                delete DefaultValueExpression;
        }
	};
}
