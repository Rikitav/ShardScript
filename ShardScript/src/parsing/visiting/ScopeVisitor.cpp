#include <shard/parsing/visiting/ScopeVisitor.h>
#include <shard/parsing/semantic/SemanticScope.h>
#include <shard/syntax/SyntaxSymbol.h>

using namespace shard::parsing;
using namespace shard::parsing::semantic;
using namespace shard::syntax;
using namespace shard::syntax::symbols;

SemanticScope* ScopeVisitor::CurrentScope()
{
	return scopeStack.top();
}

void ScopeVisitor::PushScope(const SyntaxSymbol* symbol)
{
	SemanticScope* newScope = new SemanticScope(symbol, scopeStack.top());
	scopeStack.push(newScope);
}

void ScopeVisitor::PopScope()
{
	scopeStack.pop();
}

void ScopeVisitor::Declare(SyntaxSymbol* symbol)
{
	CurrentScope()->DeclareSymbol(symbol);
}

TypeSymbol* ScopeVisitor::OwnerType()
{
	for (const SemanticScope* scope = CurrentScope(); scope != nullptr; scope = scope->Parent)
	{
		if (scope->Owner == nullptr)
			continue;

		SyntaxSymbol* symbol = const_cast<SyntaxSymbol*>(scope->Owner);
		if (symbol->IsType())
			return static_cast<TypeSymbol*>(symbol);
	}

	return nullptr;
}
