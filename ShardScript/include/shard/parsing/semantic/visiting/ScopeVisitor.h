#pragma once
#include <shard/ShardScriptAPI.h>

#include <shard/parsing/semantic/SymbolTable.h>
#include <shard/parsing/semantic/SemanticScope.h>
#include <shard/parsing/semantic/NamespaceTree.h>

#include <shard/syntax/SyntaxSymbol.h>
#include <shard/syntax/symbols/TypeSymbol.h>
#include <shard/syntax/symbols/MethodSymbol.h>
#include <shard/syntax/symbols/NamespaceSymbol.h>

#include <stack>

namespace shard
{
	class SHARD_API ScopeVisitor
	{
		std::stack<SemanticScope*> scopeStack;

	protected:
		inline ScopeVisitor(SymbolTable *const symbolTable)
		{
			scopeStack.push(SymbolTable::Global::Scope);
		}

	public:
		SemanticScope *const CurrentScope();
		void PushScopeStack(SemanticScope* scope);
		void PushScope(SyntaxSymbol *const symbol);
		void PopScope();

	protected:
		virtual void Declare(SyntaxSymbol *const symbol);
		
		virtual bool CheckNameDeclared(const std::wstring& name);
		virtual bool CheckSymbolNameDeclared(SyntaxSymbol *const symbol);
		
		SyntaxSymbol *const OwnerSymbol();
		TypeSymbol *const OwnerType();
		//NamespaceSymbol* OwnerNamespace();
		//NamespaceNode* OwnerNamespaceNode();
		MethodSymbol *const FindHostMethodSymbol();
		
		bool IsSymbolAccessible(SyntaxSymbol *const symbol);
	};
}
