#pragma once
#include <shard/ShardScriptAPI.h>

#include <shard/syntax/SyntaxKind.h>
#include <shard/syntax/SyntaxSymbol.h>
#include <shard/syntax/SymbolAccesibility.h>

#include <shard/syntax/nodes/ExpressionSyntax.h>
#include <shard/syntax/symbols/TypeSymbol.h>

#include <string>

namespace shard::syntax::symbols
{
	class SHARD_API VariableSymbol : public SyntaxSymbol
	{
	public:
		const TypeSymbol* Type = nullptr;
        shard::syntax::nodes::ExpressionSyntax* Declaration = nullptr;
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