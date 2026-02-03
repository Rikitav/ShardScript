#pragma once
#include <shard/ShardScriptAPI.h>

#include <shard/parsing/semantic/SymbolTable.h>
#include <shard/parsing/semantic/SemanticScope.h>
#include <shard/parsing/semantic/NamespaceTree.h>

#include <shard/syntax/SyntaxSymbol.h>
#include <shard/syntax/symbols/TypeSymbol.h>
#include <shard/syntax/symbols/NamespaceSymbol.h>

#include <stack>

namespace shard
{
	class SHARD_API ScopeVisitor
	{
		std::stack<shard::SemanticScope*> scopeStack;

	protected:
		inline ScopeVisitor(shard::SymbolTable *const symbolTable)
		{
			scopeStack.push(shard::SymbolTable::Global::Scope);
		}

		shard::SemanticScope *const CurrentScope();
		void PushScope(shard::SyntaxSymbol *const symbol);
		void PopScope();

		virtual bool CheckNameDeclared(const std::wstring& name);
		virtual bool CheckSymbolNameDeclared(shard::SyntaxSymbol *const symbol);
		virtual void Declare(shard::SyntaxSymbol *const symbol);
		shard::SyntaxSymbol *const OwnerSymbol();
		shard::TypeSymbol *const OwnerType();
		//shard::NamespaceSymbol* OwnerNamespace();
		//shard::NamespaceNode* OwnerNamespaceNode();
		bool IsSymbolAccessible(shard::SyntaxSymbol *const symbol);
	};
}
