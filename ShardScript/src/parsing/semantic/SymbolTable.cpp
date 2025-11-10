#include <shard/parsing/semantic/SymbolTable.h>

#include <shard/syntax/SyntaxNode.h>
#include <shard/syntax/SyntaxSymbol.h>
#include <shard/syntax/SyntaxKind.h>

#include <shard/syntax/symbols/TypeSymbol.h>

#include <vector>
#include <ranges>
#include <new>
#include <utility>

using namespace std;
using namespace std::ranges;
using namespace std::views;
using namespace shard::syntax;
using namespace shard::syntax::symbols;
using namespace shard::parsing::semantic;

void SymbolTable::ClearSymbols()
{
	for (shard::syntax::SyntaxSymbol* symbol : (symbolToNodeMap | std::views::keys))
	{
		switch (symbol->Kind)
		{
			case shard::syntax::SyntaxKind::NamespaceDeclaration:
			case shard::syntax::SyntaxKind::ClassDeclaration:
			case shard::syntax::SyntaxKind::StructDeclaration:
			{
				delete symbol;
				break;
			}
		}
	}

	symbolToNodeMap.clear();
	nodeToSymbolMap.clear();
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
}

vector<TypeSymbol*> SymbolTable::GetTypeSymbols()
{
	auto cond = [](const SyntaxSymbol* symbol) { return symbol->Kind == SyntaxKind::ClassDeclaration || symbol->Kind == SyntaxKind::StructDeclaration || symbol->Kind == SyntaxKind::CollectionExpression; };
	auto trans = [](const SyntaxSymbol* symbol) { return static_cast<TypeSymbol*>((SyntaxSymbol*)symbol); };
	auto types = symbolToNodeMap | keys | filter(cond) | transform(trans);
	return vector<TypeSymbol*>(types.begin(), types.end());
}
