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

SemanticScope *const ScopeVisitor::CurrentScope()
{
	return scopeStack.top();
}

void ScopeVisitor::PushScope(SyntaxSymbol *const symbol)
{
	SemanticScope* newScope = new SemanticScope(symbol, scopeStack.top());
	scopeStack.push(newScope);
}

void ScopeVisitor::PopScope()
{
	scopeStack.pop();
}

void ScopeVisitor::Declare(SyntaxSymbol *const symbol)
{
	CurrentScope()->DeclareSymbol(symbol);
}

bool ScopeVisitor::CheckNameDeclared(const std::wstring& name)
{
	for (SemanticScope* scope = CurrentScope(); scope != nullptr; scope = scope->Parent)
	{
		auto lookup = scope->_symbols.find(name);
		if (lookup == scope->_symbols.end())
			continue;

		return true;
	}

	return false;
}

bool ScopeVisitor::CheckSymbolNameDeclared(SyntaxSymbol *const symbol)
{
	for (SemanticScope* scope = CurrentScope(); scope != nullptr; scope = scope->Parent)
	{
		auto lookup = scope->_symbols.find(symbol->Name);
		if (lookup == scope->_symbols.end())
			continue;

		SyntaxSymbol *const found = lookup->second;
		if (found == symbol)
			continue;

		return true;
	}

	return false;
}

SyntaxSymbol *const ScopeVisitor::OwnerSymbol()
{
	for (SemanticScope* scope = CurrentScope(); scope != nullptr; scope = scope->Parent)
	{
		if (scope->Owner == nullptr)
			continue;

		SyntaxSymbol *const symbol = scope->Owner;
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

TypeSymbol *const ScopeVisitor::OwnerType()
{
	for (SemanticScope* scope = CurrentScope(); scope != nullptr; scope = scope->Parent)
	{
		if (scope->Owner == nullptr)
			continue;

		SyntaxSymbol *const symbol = scope->Owner;
		if (symbol->IsType())
			return static_cast<TypeSymbol *const>(symbol);
	}

	return nullptr;
}

static bool IsScopePublicallyAccessible(SemanticScope *const scope)
{
	if (scope == nullptr)
		return false;

	if (scope->Owner->Kind == SyntaxKind::NamespaceDeclaration)
		return true;

	if (scope->Owner->Accesibility == SymbolAccesibility::Public)
		return true;

	return IsScopePublicallyAccessible(scope->Parent);
}

static bool IsSymbolNestedAccessible(const SemanticScope* scope, SyntaxSymbol *const symbol)
{
	if (scope == nullptr)
		return true;

	if (scope->Owner->Kind == SyntaxKind::NamespaceDeclaration)
		return false;

	if (scope->Owner == symbol->Parent)
		return true;

	return IsSymbolNestedAccessible(scope->Parent, symbol);
}

bool ScopeVisitor::IsSymbolAccessible(SyntaxSymbol *const symbol)
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

