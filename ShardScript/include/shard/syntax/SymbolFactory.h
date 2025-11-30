#pragma once
#include <shard/syntax/nodes/MemberDeclarations/MethodDeclarationSyntax.h>

#include <shard/syntax/symbols/MethodSymbol.h>

namespace shard::syntax
{
	class SymbolFactory
	{
	public:
		// Methods
		static shard::syntax::symbols::MethodSymbol* Method(shard::syntax::nodes::MethodDeclarationSyntax* node);
	};
}