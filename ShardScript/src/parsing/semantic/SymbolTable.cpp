#include <shard/parsing/semantic/SymbolTable.hpp>

#include <shard/syntax/SyntaxNode.hpp>
#include <shard/syntax/SyntaxSymbol.hpp>
#include <shard/syntax/SyntaxKind.hpp>

#include <shard/syntax/symbols/TypeSymbol.hpp>
#include <shard/syntax/symbols/NamespaceSymbol.hpp>

#include <vector>
#include <ranges>
#include <new>
#include <utility>

using namespace std::ranges;
using namespace std::views;
using namespace shard;

TypeSymbol* const SymbolTable::Global::Type = new TypeSymbol(GlobalTypeName, SyntaxKind::CompilationUnit);
SemanticScope* const SymbolTable::Global::Scope = new SemanticScope(Type, nullptr);

SymbolTable::SymbolTable()
{

}

SymbolTable::~SymbolTable()
{
	ClearSymbols();
}

void SymbolTable::ClearSymbols()
{
	for (shard::SyntaxSymbol* symbol : (symbolToNodeMap | std::views::keys))
	{
		if (symbol->IsType())
			delete symbol;
	}

	symbolToNodeMap.clear();
	nodeToSymbolMap.clear();
	namespacesList.clear();
	typesList.clear();
}

SyntaxSymbol *const SymbolTable::LookupSymbol(SyntaxNode *const node)
{
	auto choise = nodeToSymbolMap.find(node);
	return choise == nodeToSymbolMap.end() ? nullptr : choise->second;
}

SyntaxNode *const SymbolTable::GetSyntaxNode(SyntaxSymbol * const symbol)
{
	auto choise = symbolToNodeMap.find(symbol);
	return choise == symbolToNodeMap.end() ? nullptr : choise->second;
}

void SymbolTable::BindSymbol(SyntaxNode* node, SyntaxSymbol* symbol)
{
	nodeToSymbolMap[node] = symbol;
	symbolToNodeMap[symbol] = node;

	if (symbol->IsType())
		typesList.push_back(static_cast<TypeSymbol*>(symbol));

	if (symbol->Kind == SyntaxKind::NamespaceDeclaration)
		namespacesList.push_back(static_cast<NamespaceSymbol*>(symbol));
}

const std::vector<NamespaceSymbol*> SymbolTable::GetNamespaceSymbols()
{
	return namespacesList;
}

const std::vector<TypeSymbol*> SymbolTable::GetTypeSymbols()
{
	/*
	auto cond = [](const SyntaxSymbol* symbol) { return symbol->Kind == SyntaxKind::ClassDeclaration || symbol->Kind == SyntaxKind::StructDeclaration || symbol->Kind == SyntaxKind::CollectionExpression; };
	auto trans = [](const SyntaxSymbol* symbol) { return static_cast<TypeSymbol*>((SyntaxSymbol*)symbol); };
	auto types = symbolToNodeMap | keys | filter(cond) | transform(trans);
	return std::vector<TypeSymbol*>(types.begin(), types.end());
	*/

	return typesList;
}
