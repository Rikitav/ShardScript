#pragma once
#include <shard/parsing/semantic/SymbolTable.h>
#include <shard/parsing/semantic/SemanticScope.h>
#include <shard/syntax/SyntaxSymbol.h>
#include <shard/syntax/symbols/TypeSymbol.h>
#include <stack>

namespace shard::parsing
{
	class ScopeVisitor
	{
		std::stack<shard::parsing::semantic::SemanticScope*> scopeStack;

	protected:
		inline ScopeVisitor(shard::parsing::semantic::SymbolTable* symbolTable)
		{
			scopeStack.push(symbolTable->GlobalScope);
		}

		shard::parsing::semantic::SemanticScope* CurrentScope();
		void PushScope(const shard::syntax::SyntaxSymbol* symbol);
		void PopScope();

		void Declare(shard::syntax::SyntaxSymbol* symbol);
		shard::syntax::symbols::TypeSymbol* OwnerType();
	};
}
