#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/parsing/SyntaxKind.hpp>
#include <shard/semantic/SyntaxSymbol.hpp>

#include <shard/parsing/nodes/ExpressionSyntax.hpp>
#include <shard/semantic/symbols/TypeSymbol.hpp>

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