#pragma once
#include <shard/syntax/SyntaxKind.h>
#include <shard/syntax/SyntaxSymbol.h>
#include <shard/syntax/SymbolAccesibility.h>
#include <shard/syntax/nodes/ExpressionSyntax.h>
#include <shard/syntax/symbols/TypeSymbol.h>
#include <string>

namespace shard::syntax::symbols
{
	class VariableSymbol : public SyntaxSymbol
	{
	public:
		TypeSymbol* Type = nullptr;
        shard::syntax::nodes::ExpressionSyntax* Declaration = nullptr;
        bool IsConst = false;

		inline VariableSymbol(std::wstring name) : SyntaxSymbol(name, SyntaxKind::VariableStatement)
		{
			Accesibility = SymbolAccesibility::Public;
		}
	};
}