#include <shard/parsing/semantic/SymbolTable.hpp>

#include <shard/syntax/SyntaxNode.hpp>
#include <shard/syntax/SyntaxSymbol.hpp>
#include <shard/syntax/SyntaxKind.hpp>

#include <shard/syntax/symbols/TypeSymbol.hpp>
#include <shard/syntax/symbols/StructSymbol.hpp>
#include <shard/syntax/symbols/ClassSymbol.hpp>
#include <shard/syntax/symbols/NamespaceSymbol.hpp>

#include <vector>
#include <ranges>
#include <new>
#include <utility>
#include <optional>

using namespace std::ranges;
using namespace std::views;
using namespace shard;

TypeSymbol* const SymbolTable::Global::Type = new TypeSymbol(GlobalTypeName, SyntaxKind::CompilationUnit);
SemanticScope* const SymbolTable::Global::Scope = new SemanticScope(Type, nullptr);

template<typename B, typename D>
static std::unique_ptr<D> unique_cast(std::unique_ptr<B> ptr)
{
	D* tmp = static_cast<D*>(ptr.get());
	std::unique_ptr<D> derivedPointer;

	if (tmp != nullptr)
	{
		ptr.release();
		derivedPointer.reset(tmp);
	}

	return derivedPointer;
}

static void ResolvePrimitives()
{
	SymbolTable::Primitives::Void = new StructSymbol(L"Void");
	SymbolTable::Primitives::Any = new StructSymbol(L"Any");

	SymbolTable::Primitives::Void->State = TypeLayoutingState::Visited;
	SymbolTable::Primitives::Any->State = TypeLayoutingState::Visited;

	SymbolTable::Primitives::Void->MemoryBytesSize = 0;
	SymbolTable::Primitives::Any->MemoryBytesSize = 0;

	SymbolTable::Primitives::Boolean = new StructSymbol(L"Boolean");
	SymbolTable::Primitives::Integer = new StructSymbol(L"Integer");
	SymbolTable::Primitives::Double = new StructSymbol(L"Double");
	SymbolTable::Primitives::Char = new StructSymbol(L"Char");
	SymbolTable::Primitives::String = new ClassSymbol(L"String");
	SymbolTable::Primitives::Array = new ClassSymbol(L"Array");

	SymbolTable::Primitives::Boolean->State = TypeLayoutingState::Visited;
	SymbolTable::Primitives::Integer->State = TypeLayoutingState::Visited;
	SymbolTable::Primitives::Double->State = TypeLayoutingState::Visited;
	SymbolTable::Primitives::Char->State = TypeLayoutingState::Visited;
	SymbolTable::Primitives::String->State = TypeLayoutingState::Visited;
	SymbolTable::Primitives::Array->State = TypeLayoutingState::Visited;

	SymbolTable::Primitives::Boolean->MemoryBytesSize = sizeof(bool);
	SymbolTable::Primitives::Integer->MemoryBytesSize = sizeof(std::int64_t);
	SymbolTable::Primitives::Double->MemoryBytesSize = sizeof(double);
	SymbolTable::Primitives::Char->MemoryBytesSize = sizeof(wchar_t);
	SymbolTable::Primitives::String->MemoryBytesSize = sizeof(std::int64_t) + sizeof(wchar_t*); // long _length + char[] _data
	SymbolTable::Primitives::Array->MemoryBytesSize = sizeof(std::int64_t);					   // long _length
}

SymbolTable::SymbolTable()
{
	if (SymbolTable::Primitives::Void == nullptr)
		ResolvePrimitives();
}

SymbolTable::~SymbolTable()
{
	ClearSymbols();
}

void SymbolTable::ClearSymbols()
{
	symbolToNodeMap.clear();
	nodeToSymbolMap.clear();

	namespacesList.clear();
	typesList.clear();
	membersList.clear();
	triviasList.clear();
}

std::optional<SyntaxSymbol*> SymbolTable::LookupSymbol(SyntaxNode *const node)
{
	auto choise = nodeToSymbolMap.find(node);
	return choise == nodeToSymbolMap.end() ? std::nullopt : std::optional<SyntaxSymbol*>(choise->second);
}

std::optional<SyntaxNode*> SymbolTable::LookupNode(SyntaxSymbol * const symbol)
{
	auto choise = symbolToNodeMap.find(symbol);
	return choise == symbolToNodeMap.end() ? std::nullopt : std::optional<SyntaxNode*>(choise->second);
}

SyntaxSymbol* SymbolTable::BindSymbol(SyntaxNode* node, std::unique_ptr<SyntaxSymbol> symbol)
{
	SyntaxSymbol* raw = symbol.get();
	nodeToSymbolMap[node] = raw;
	symbolToNodeMap[raw] = node;

	if (symbol->Kind == SyntaxKind::NamespaceDeclaration)
	{
		namespacesList.push_back(unique_cast<SyntaxSymbol, NamespaceSymbol>(std::move(symbol)));
	}
	else if (symbol->IsType())
	{
		typesList.push_back(unique_cast<SyntaxSymbol, TypeSymbol>(std::move(symbol)));
	}
	else if (symbol->IsMember())
	{
		membersList.push_back(unique_cast<SyntaxSymbol, MemberSymbol>(std::move(symbol)));
	}
	else
	{
		triviasList.push_back(std::move(symbol));
	}

	return raw;
}

SyntaxSymbol* SymbolTable::ImplicitSymbol(std::unique_ptr<SyntaxSymbol> symbol)
{
	SyntaxSymbol* raw = symbol.get();
	if (symbol->Kind == SyntaxKind::NamespaceDeclaration)
	{
		namespacesList.push_back(unique_cast<SyntaxSymbol, NamespaceSymbol>(std::move(symbol)));
	}
	else if (symbol->IsType())
	{
		typesList.push_back(unique_cast<SyntaxSymbol, TypeSymbol>(std::move(symbol)));
	}
	else if (symbol->IsMember())
	{
		membersList.push_back(unique_cast<SyntaxSymbol, MemberSymbol>(std::move(symbol)));
	}
	else
	{
		triviasList.push_back(std::move(symbol));
	}

	return raw;
}

const std::vector<NamespaceSymbol*> SymbolTable::GetNamespaceSymbols()
{
	std::vector<NamespaceSymbol*> result;
	result.reserve(namespacesList.size());
	for (const auto& symbol : namespacesList)
		result.push_back(symbol.get());
	return result;
}

const std::vector<TypeSymbol*> SymbolTable::GetTypeSymbols()
{
	std::vector<TypeSymbol*> result;
	result.reserve(typesList.size());
	for (const auto& symbol : typesList)
		result.push_back(symbol.get());
	return result;
}

const std::vector<MethodSymbol*> shard::SymbolTable::GetMethodSymbols()
{
	std::vector<MethodSymbol*> methods;
	for (const auto& member : membersList)
	{
		if (member->Kind == SyntaxKind::MethodDeclaration)
			methods.push_back(static_cast<MethodSymbol*>(member.get()));
	}

	return methods;
}