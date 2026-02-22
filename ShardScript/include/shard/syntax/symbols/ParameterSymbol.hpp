#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/syntax/symbols/TypeSymbol.hpp>
#include <shard/syntax/nodes/ExpressionSyntax.hpp>

#include <shard/syntax/SymbolAccesibility.hpp>
#include <shard/syntax/SyntaxKind.hpp>
#include <shard/syntax/SyntaxSymbol.hpp>

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
