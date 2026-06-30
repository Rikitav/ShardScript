#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/semantic/symbols/TypeSymbol.hpp>
#include <shard/parsing/nodes/ExpressionSyntax.hpp>

#include <shard/parsing/SyntaxKind.hpp>
#include <shard/semantic/SyntaxSymbol.hpp>

#include <string>
#include <cstdint>
#include <memory>

namespace shard
{
	class SHARD_API ParameterSymbol : public SyntaxSymbol
	{
    public:
        ExpressionSyntax* DefaultValueExpression = nullptr;
        TypeSymbol* Type = nullptr;

        std::uint16_t SlotIndex = 0;
        bool IsOptional = false;

        inline ParameterSymbol(const std::wstring& name) : SyntaxSymbol(name, SyntaxKind::Parameter)
        {
            Accesibility = SymbolAccesibility::Public;
        }

        inline ParameterSymbol(const std::wstring& name, TypeSymbol* type) : SyntaxSymbol(name, SyntaxKind::Parameter), Type(type)
        {
            Accesibility = SymbolAccesibility::Public;
        }

        inline ParameterSymbol(const ParameterSymbol& other) = delete;

        inline virtual ~ParameterSymbol() = default;
	};
}
