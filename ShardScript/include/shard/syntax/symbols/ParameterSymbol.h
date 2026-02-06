#pragma once
#include <shard/ShardScriptAPI.h>

#include <shard/syntax/symbols/TypeSymbol.h>
#include <shard/syntax/nodes/ExpressionSyntax.h>

#include <shard/syntax/SymbolAccesibility.h>
#include <shard/syntax/SyntaxKind.h>
#include <shard/syntax/SyntaxSymbol.h>

#include <string>
#include <cstdint>

namespace shard
{
	class SHARD_API ParameterSymbol : public SyntaxSymbol
	{
    public:
        TypeSymbol* Type = nullptr;
        shard::ExpressionSyntax* DefaultValueExpression = nullptr;

        uint16_t SlotIndex = 0;
        bool IsOptional = false;

        inline ParameterSymbol(const std::wstring name) : SyntaxSymbol(name, SyntaxKind::Parameter)
        {
            Accesibility = SymbolAccesibility::Public;
        }

        inline ParameterSymbol(const std::wstring name, TypeSymbol* type) : SyntaxSymbol(name, SyntaxKind::Parameter), Type(type)
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
