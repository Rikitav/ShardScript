#pragma once
#include <shard/ShardScriptAPI.hpp>

#include <shard/parsing/semantic/SymbolTable.hpp>
#include <shard/parsing/semantic/SemanticScope.hpp>
#include <shard/parsing/semantic/NamespaceTree.hpp>

#include <shard/syntax/SyntaxSymbol.hpp>
#include <shard/syntax/symbols/TypeSymbol.hpp>
#include <shard/syntax/symbols/MethodSymbol.hpp>
#include <shard/syntax/symbols/NamespaceSymbol.hpp>

#include <stack>

namespace shard
{
	class SHARD_API ScopeVisitor
	{
		std::stack<SemanticScope*> scopeStack;

	protected:
		inline ScopeVisitor(SymbolTable* symbolTable)
		{
			scopeStack.push(SymbolTable::Global::Scope);
		}

	public:
		SemanticScope* CurrentScope();
		void PushScopeStack(SemanticScope* scope);
		void PushScope(SyntaxSymbol* symbol);
		void PopScope();

	protected:
		virtual void Declare(SyntaxSymbol* symbol);
		
		virtual bool CheckNameDeclared(const std::wstring& name);
		virtual bool CheckNameDeclared(SyntaxSymbol* symbol);
		
		std::optional<SyntaxSymbol*> OwnerSymbol();
		std::optional<TypeSymbol*> OwnerType();
		std::optional<MethodSymbol*> FindHostMethodSymbol();
		
		bool IsSymbolAccessible(SyntaxSymbol* symbol, SyntaxNode* symbolDeclaringNode, SyntaxNode* callSiteNode);
	};
}
