#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/syntax/SyntaxKind.hpp>
#include <shard/syntax/SyntaxSymbol.hpp>
#include <shard/syntax/SymbolAccesibility.hpp>

#include <shard/syntax/nodes/ExpressionSyntax.hpp>
#include <shard/syntax/symbols/TypeSymbol.hpp>

#include <string>

namespace shard
{
	class SHARD_API VariableSymbol : public SyntaxSymbol
	{
	public:
		const TypeSymbol* Type = nullptr;
        shard::ExpressionSyntax* Declaration = nullptr;

		uint16_t SlotIndex = 0;
        bool IsConst = false;

		inline VariableSymbol(std::wstring name, TypeSymbol* type) : SyntaxSymbol(name, SyntaxKind::VariableStatement), Type(type)
		{
			Accesibility = SymbolAccesibility::Public;
		}

		inline VariableSymbol(const VariableSymbol& other) = delete;

		inline virtual ~VariableSymbol() override
		{
			if (Declaration == nullptr)
			{
				delete Declaration;
				Declaration = nullptr;
			}
		}
	};
}