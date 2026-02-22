#include <shard/parsing/semantic/visiting/ScopeVisitor.hpp>
#include <shard/parsing/semantic/SemanticScope.hpp>
#include <shard/parsing/semantic/NamespaceTree.hpp>

#include <shard/syntax/SyntaxSymbol.hpp>
#include <shard/syntax/SymbolAccesibility.hpp>
#include <shard/syntax/SyntaxKind.hpp>

#include <shard/syntax/symbols/NamespaceSymbol.hpp>
#include <shard/syntax/symbols/TypeSymbol.hpp>

#include <stdexcept>

using namespace shard;

SemanticScope *const ScopeVisitor::CurrentScope()
{
	return scopeStack.top();
}

void ScopeVisitor::PushScopeStack(SemanticScope* const scope)
{
	scopeStack.push(scope);
}

void ScopeVisitor::PushScope(SyntaxSymbol *const symbol)
{
	SemanticScope* newScope = new SemanticScope(symbol, CurrentScope());
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

	if (scope->Owner == nullptr)
		return true;

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

static bool IsMethodSymbol(SyntaxSymbol *const symbol)
{
	switch (symbol->Kind)
	{
		default:
			return false;

		case SyntaxKind::MethodDeclaration:
		case SyntaxKind::ConstructorDeclaration:
		case SyntaxKind::AccessorDeclaration:
			return true;
	}
}

MethodSymbol *const shard::ScopeVisitor::FindHostMethodSymbol()
{
	for (SemanticScope* scope = CurrentScope(); scope != nullptr; scope = scope->Parent)
	{
		if (IsMethodSymbol(scope->Owner))
			return static_cast<MethodSymbol *const>(scope->Owner);
	}

	return nullptr;
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

