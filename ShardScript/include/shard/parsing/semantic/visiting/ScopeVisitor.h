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
		inline ScopeVisitor(shard::SymbolTable* symbolTable)
		{
			scopeStack.push(shard::SymbolTable::Global::Scope);
		}

		shard::SemanticScope* CurrentScope();
		void PushScope(const shard::SyntaxSymbol* symbol);
		void PopScope();

		virtual bool CheckNameDeclared(std::wstring& name);
		virtual bool CheckSymbolNameDeclared(shard::SyntaxSymbol* symbol);
		virtual void Declare(shard::SyntaxSymbol* symbol);
		shard::SyntaxSymbol* OwnerSymbol();
		shard::TypeSymbol* OwnerType();
		//shard::NamespaceSymbol* OwnerNamespace();
		//shard::NamespaceNode* OwnerNamespaceNode();
		bool IsSymbolAccessible(shard::SyntaxSymbol* symbol);
	};
}
