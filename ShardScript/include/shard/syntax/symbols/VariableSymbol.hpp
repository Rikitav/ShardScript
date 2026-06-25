#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/syntax/SyntaxKind.hpp>
#include <shard/syntax/SyntaxSymbol.hpp>

#include <shard/syntax/nodes/ExpressionSyntax.hpp>
#include <shard/syntax/symbols/TypeSymbol.hpp>

#include <cstdint>
#include <string>
#include <memory>

namespace shard
{
	class SHARD_API VariableSymbol : public SyntaxSymbol
	{
	public:
		const TypeSymbol* Type = nullptr;
        ExpressionSyntax* Declaration = nullptr;

		std::uint16_t SlotIndex = 0;
        bool IsConst = false;

		inline VariableSymbol(const std::wstring& name) : SyntaxSymbol(name, SyntaxKind::VariableStatement)
		{
			Accesibility = SymbolAccesibility::Public;
		}

		inline VariableSymbol(const std::wstring& name, TypeSymbol* type) : SyntaxSymbol(name, SyntaxKind::VariableStatement), Type(type)
		{
			Accesibility = SymbolAccesibility::Public;
		}

		inline VariableSymbol(const VariableSymbol& other) = delete;

		inline virtual ~VariableSymbol() = default;
	};
}