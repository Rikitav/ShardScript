#include <shard/parsing/semantic/SymbolTable.h>

#include <shard/parsing/semantic/primitives/BooleanPrimitive.h>
#include <shard/parsing/semantic/primitives/IntegerPrimitive.h>
#include <shard/parsing/semantic/primitives/CharPrimitive.h>
#include <shard/parsing/semantic/primitives/StringPrimitive.h>

#include <shard/syntax/SyntaxNode.h>
#include <shard/syntax/SyntaxSymbol.h>
#include <shard/syntax/SyntaxKind.h>

#include <shard/syntax/symbols/ClassSymbol.h>
#include <shard/syntax/symbols/StructSymbol.h>
#include <shard/syntax/symbols/TypeSymbol.h>

#include <vector>
#include <ranges>

using namespace std;
using namespace std::ranges;
using namespace std::views;
using namespace shard::syntax;
using namespace shard::syntax::symbols;
using namespace shard::parsing::semantic;

void SymbolTable::ResolvePrmitives()
{
	Primitives::Void = new SyntaxSymbol(L"void", SyntaxKind::Unknown);

	Primitives::Boolean = new StructSymbol(L"Boolean");
	Primitives::Integer = new StructSymbol(L"Integer");
	Primitives::Char = new StructSymbol(L"Char");
	Primitives::String = new ClassSymbol(L"String");

	GlobalScope->DeclareSymbol(Primitives::Boolean);
	GlobalScope->DeclareSymbol(Primitives::Integer);
	GlobalScope->DeclareSymbol(Primitives::Char);
	GlobalScope->DeclareSymbol(Primitives::String);

	BooleanPrimitive::Reflect(Primitives::Boolean);
	IntegerPrimitive::Reflect(Primitives::Integer);
	//CharPrimitive::Reflect(Primitives::Char);
	StringPrimitive::Reflect(Primitives::String);
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
	auto cond = [](const SyntaxSymbol* symbol) { return symbol->Kind == SyntaxKind::ClassDeclaration || symbol->Kind == SyntaxKind::StructDeclaration; };
	auto trans = [](const SyntaxSymbol* symbol) { return static_cast<TypeSymbol*>((SyntaxSymbol*)symbol); };
	auto types = symbolToNodeMap | keys | filter(cond) | transform(trans);
	return vector<TypeSymbol*>(types.begin(), types.end());
}
