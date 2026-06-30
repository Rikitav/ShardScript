#include <shard/semantic/ScopeVisitor.hpp>
#include <shard/semantic/SemanticScope.hpp>
#include <shard/semantic/NamespaceTree.hpp>

#include <shard/semantic/SyntaxSymbol.hpp>
#include <shard/parsing/SyntaxKind.hpp>

#include <shard/semantic/symbols/NamespaceSymbol.hpp>
#include <shard/semantic/symbols/TypeSymbol.hpp>

#include <shard/parsing/nodes/CompilationUnitSyntax.hpp>

#include <stdexcept>

using namespace shard;

SemanticScope* ScopeVisitor::CurrentScope()
{
	return scopeStack.top();
}

void ScopeVisitor::PushScopeStack(SemanticScope* scope)
{
	scopeStack.push(scope);
}

void ScopeVisitor::PushScope(SyntaxSymbol* symbol)
{
	SemanticScope* newScope = new SemanticScope(symbol, CurrentScope());
	scopeStack.push(newScope);
}

void ScopeVisitor::PopScope()
{
	SemanticScope* scope = scopeStack.top();
	scopeStack.pop();
	delete scope;
}

void ScopeVisitor::Declare(SyntaxSymbol* symbol)
{
	SemanticScope* scope = CurrentScope();
	scope->DeclareSymbol(symbol);

	if (scope->Owner != nullptr)
		scope->Owner->OnSymbolDeclared(symbol);
}

bool ScopeVisitor::CheckNameDeclared(const std::wstring& name)
{
	for (SemanticScope* scope = CurrentScope(); scope != nullptr; scope = scope->Parent)
	{
		auto lookup = scope->_symbols.find(name);
		if (lookup != scope->_symbols.end())
			return true;
	}
	
	return false;
}

bool ScopeVisitor::CheckNameDeclared(SyntaxSymbol* symbol)
{
	for (SemanticScope* scope = CurrentScope(); scope != nullptr; scope = scope->Parent)
	{
		auto lookup = scope->_symbols.find(symbol->Name);
		if (lookup != scope->_symbols.end())
		{
			if (lookup->second != symbol)
				return true;
		}
	}
	
	return false;
}

std::optional<SyntaxSymbol*> ScopeVisitor::OwnerSymbol()
{
	for (SemanticScope* scope = CurrentScope(); scope != nullptr; scope = scope->Parent)
	{
		if (scope->Owner == nullptr)
			continue;

		SyntaxSymbol* symbol = scope->Owner;
		if (symbol->IsMember() || symbol->IsType() || symbol->Kind == SyntaxKind::NamespaceDeclaration)
			return symbol;
	}

	return std::nullopt;
}

std::optional<TypeSymbol*> ScopeVisitor::OwnerType()
{
	for (SemanticScope* scope = CurrentScope(); scope != nullptr; scope = scope->Parent)
	{
		if (scope->Owner == nullptr)
			continue;

		if (scope->Owner->IsType())
			return static_cast<TypeSymbol*>(scope->Owner);
	}

	return std::nullopt;
}

static CompilationUnitSyntax* GetCompilationUnit(SyntaxNode* node)
{
	while (node != nullptr && node->Kind != SyntaxKind::CompilationUnit)
		node = node->Parent;

	return static_cast<CompilationUnitSyntax*>(node);
}

static bool IsMethodSymbol(SyntaxSymbol* symbol)
{
	if (symbol == nullptr)
		return false;

	switch (symbol->Kind)
	{
		case SyntaxKind::MethodDeclaration:
		case SyntaxKind::ConstructorDeclaration:
		case SyntaxKind::AccessorDeclaration:
			return true;

		default:
			return false;
	}
}

std::optional<MethodSymbol*> ScopeVisitor::FindHostMethodSymbol()
{
	for (SemanticScope* scope = CurrentScope(); scope != nullptr; scope = scope->Parent)
	{
		if (IsMethodSymbol(scope->Owner))
			return static_cast<MethodSymbol*>(scope->Owner);
	}

	return std::nullopt;
}

bool ScopeVisitor::IsSymbolAccessible(SyntaxSymbol* symbol, SyntaxNode* symbolDeclaringNode, SyntaxNode* callSiteNode)
{
	if (symbol == nullptr)
		return true;

	if (symbol->Kind == SyntaxKind::NamespaceDeclaration)
		return true;

	if (symbol->Accesibility == SymbolAccesibility::Public)
		return true;

	if (symbol->Parent == nullptr)
		throw std::runtime_error("Cannot resolve symbol without parent");

	if (symbol->Parent->Kind == SyntaxKind::NamespaceDeclaration || symbol->Parent->Kind == SyntaxKind::CompilationUnit)
	{
		if (callSiteNode == nullptr)
			return false;

		CompilationUnitSyntax* declarationFile = GetCompilationUnit(symbolDeclaringNode);
		CompilationUnitSyntax* currentFile = GetCompilationUnit(callSiteNode);
		return declarationFile != nullptr && declarationFile == currentFile;
	}

	for (SemanticScope* scope = CurrentScope(); scope != nullptr; scope = scope->Parent)
	{
		if (scope->Owner == symbol->Parent)
			return true;
	}

	return false;
}
