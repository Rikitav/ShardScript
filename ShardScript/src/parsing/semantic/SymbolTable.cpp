#include <shard/parsing/semantic/SymbolTable.h>

#include <shard/syntax/SyntaxNode.h>
#include <shard/syntax/SyntaxSymbol.h>
#include <shard/syntax/SyntaxKind.h>

#include <shard/syntax/symbols/TypeSymbol.h>
#include <shard/syntax/symbols/NamespaceSymbol.h>

#include <vector>
#include <ranges>
#include <new>
#include <utility>

using namespace std::ranges;
using namespace std::views;
using namespace shard;

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
	EntryPointCandidates.clear();
}

SyntaxSymbol* SymbolTable::LookupSymbol(SyntaxNode* node)
{
	auto choise = nodeToSymbolMap.find(node);
	return choise == nodeToSymbolMap.end() ? nullptr : choise->second;
}

SyntaxNode* SymbolTable::GetSyntaxNode(SyntaxSymbol* symbol)
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

std::vector<NamespaceSymbol*> SymbolTable::GetNamespaceSymbols()
{
	return namespacesList;
}

std::vector<TypeSymbol*> SymbolTable::GetTypeSymbols()
{
	/*
	auto cond = [](const SyntaxSymbol* symbol) { return symbol->Kind == SyntaxKind::ClassDeclaration || symbol->Kind == SyntaxKind::StructDeclaration || symbol->Kind == SyntaxKind::CollectionExpression; };
	auto trans = [](const SyntaxSymbol* symbol) { return static_cast<TypeSymbol*>((SyntaxSymbol*)symbol); };
	auto types = symbolToNodeMap | keys | filter(cond) | transform(trans);
	return std::vector<TypeSymbol*>(types.begin(), types.end());
	*/

	return typesList;
}
