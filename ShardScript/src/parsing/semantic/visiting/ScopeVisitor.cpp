#include <shard/parsing/semantic/visiting/ScopeVisitor.h>
#include <shard/parsing/semantic/SemanticScope.h>
#include <shard/parsing/semantic/NamespaceTree.h>

#include <shard/syntax/SyntaxSymbol.h>
#include <shard/syntax/SymbolAccesibility.h>
#include <shard/syntax/SyntaxKind.h>

#include <shard/syntax/symbols/NamespaceSymbol.h>
#include <shard/syntax/symbols/TypeSymbol.h>

#include <stdexcept>

using namespace shard;

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

bool ScopeVisitor::CheckNameDeclared(std::wstring& name)
{
	for (const SemanticScope* scope = CurrentScope(); scope != nullptr; scope = scope->Parent)
	{
		auto lookup = scope->_symbols.find(name);
		if (lookup == scope->_symbols.end())
			continue;

		return true;
	}

	return false;
}

bool ScopeVisitor::CheckSymbolNameDeclared(SyntaxSymbol* symbol)
{
	for (const SemanticScope* scope = CurrentScope(); scope != nullptr; scope = scope->Parent)
	{
		auto lookup = scope->_symbols.find(symbol->Name);
		if (lookup == scope->_symbols.end())
			continue;

		SyntaxSymbol* found = lookup->second;
		if (found == symbol)
			continue;

		return true;
	}

	return false;
}

SyntaxSymbol* ScopeVisitor::OwnerSymbol()
{
	for (const SemanticScope* scope = CurrentScope(); scope != nullptr; scope = scope->Parent)
	{
		if (scope->Owner == nullptr)
			continue;

		SyntaxSymbol* symbol = const_cast<SyntaxSymbol*>(scope->Owner);
		if (symbol->IsMember() || symbol->IsType() || symbol->Kind == SyntaxKind::NamespaceDeclaration)
			return symbol;
	}

	return nullptr;
}

/*
NamespaceSymbol* ScopeVisitor::OwnerNamespace()
{
	for (const SemanticScope* scope = CurrentScope(); scope != nullptr; scope = scope->Parent)
	{
		if (scope->Owner == nullptr)
			continue;

		SyntaxSymbol* symbol = const_cast<SyntaxSymbol*>(scope->Owner);
		if (symbol->Kind == SyntaxKind::NamespaceDeclaration)
			return static_cast<NamespaceSymbol*>(symbol);
	}

	return nullptr;
}

NamespaceNode* ScopeVisitor::OwnerNamespaceNode()
{
	for (const SemanticScope* scope = CurrentScope(); scope != nullptr; scope = scope->Parent)
	{
		if (scope->Namespace != nullptr)
			return scope->Namespace;
	}

	return nullptr;
}
*/

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

static bool IsScopePublicallyAccessible(const SemanticScope* scope)
{
	if (scope == nullptr)
		return false;

	if (scope->Owner->Kind == SyntaxKind::NamespaceDeclaration)
		return true;

	if (scope->Owner->Accesibility == SymbolAccesibility::Public)
		return true;

	return IsScopePublicallyAccessible(scope->Parent);
}

static bool IsSymbolNestedAccessible(const SemanticScope* scope, SyntaxSymbol* symbol)
{
	if (scope == nullptr)
		return true;

	if (scope->Owner->Kind == SyntaxKind::NamespaceDeclaration)
		return false;

	if (scope->Owner == symbol->Parent)
		return true;

	return IsSymbolNestedAccessible(scope->Parent, symbol);
}

bool ScopeVisitor::IsSymbolAccessible(SyntaxSymbol* symbol)
{
	if (symbol == nullptr)
		return true;

	if (symbol->Kind == SyntaxKind::NamespaceDeclaration)
		return true;

	if (IsScopePublicallyAccessible(CurrentScope()) && symbol->Accesibility == SymbolAccesibility::Public)
		return true;

	if (symbol->Parent == nullptr)
		throw std::runtime_error("Cannot resolve symbol without parent");

	if (IsSymbolNestedAccessible(CurrentScope(), symbol))
		return true;

	return false;
}

