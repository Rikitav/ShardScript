#include <shard/parsing/SyntaxNode.hpp>
#include <shard/parsing/SyntaxKind.hpp>

#include <shard/semantic/SymbolTable.hpp>
#include <shard/semantic/SyntaxSymbol.hpp>
#include <shard/semantic/SymbolFactory.hpp>
#include <shard/semantic/SymbolBuilder.hpp>

#include <shard/semantic/symbols/TypeSymbol.hpp>
#include <shard/semantic/symbols/StructSymbol.hpp>
#include <shard/semantic/symbols/ClassSymbol.hpp>
#include <shard/semantic/symbols/InterfaceSymbol.hpp>
#include <shard/semantic/symbols/NamespaceSymbol.hpp>
#include <shard/semantic/symbols/FieldSymbol.hpp>
#include <shard/semantic/symbols/MethodSymbol.hpp>
#include <shard/semantic/symbols/PropertySymbol.hpp>
#include <shard/semantic/symbols/AccessorSymbol.hpp>
#include <shard/semantic/symbols/ConstructorSymbol.hpp>
#include <shard/semantic/symbols/TypeParameterSymbol.hpp>
#include <shard/semantic/symbols/ArrayTypeSymbol.hpp>
#include <shard/semantic/symbols/GenericTypeSymbol.hpp>

#include <shard/runtime/EventLoop.hpp>
#include <shard/runtime/ObjectInstance.hpp>
#include <shard/runtime/GarbageCollector.hpp>
#include <shard/runtime/MethodCallState.hpp>

#include <shard/compilation/ByteCodeEncoder.hpp>

#include <vector>
#include <ranges>
#include <new>
#include <utility>
#include <optional>
#include <sstream>
#include <memory>

using namespace std::ranges;
using namespace std::views;
using namespace shard;

NamespaceSymbol* SymbolTable::Global::Namespace = new NamespaceSymbol(GlobalTypeName);
SemanticScope* SymbolTable::Global::Scope = new SemanticScope(Namespace, nullptr);

namespace
{
	template<typename B, typename D>
	static inline std::unique_ptr<D> unique_cast(std::unique_ptr<B> ptr)
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
}

SymbolTable::SymbolTable()
{
	ResolveGlobalComponents(this);
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

std::optional<SyntaxSymbol*> SymbolTable::LookupSymbol(SyntaxNode* node)
{
	auto choise = nodeToSymbolMap.find(node);
	return choise == nodeToSymbolMap.end() ? std::nullopt : std::optional<SyntaxSymbol*>(choise->second);
}

std::optional<SyntaxNode*> SymbolTable::LookupNode(SyntaxSymbol * symbol)
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

void SymbolTable::MarkAllSymbolsReady()
{
	for (const auto& symbol : namespacesList)
		symbol->AdvanceAnalysisState(SymbolAnalysisState::Ready);

	for (const auto& symbol : typesList)
		symbol->AdvanceAnalysisState(SymbolAnalysisState::Ready);

	for (const auto& symbol : membersList)
		symbol->AdvanceAnalysisState(SymbolAnalysisState::Ready);

	for (const auto& symbol : triviasList)
		symbol->AdvanceAnalysisState(SymbolAnalysisState::Ready);
}

void SymbolTable::MarkJustCreatedSymbolsReady()
{
	for (const auto& symbol : namespacesList)
	{
		if (symbol->AnalysisState == SymbolAnalysisState::JustCreated)
			symbol->AdvanceAnalysisState(SymbolAnalysisState::Ready);
	}

	for (const auto& symbol : typesList)
	{
		if (symbol->AnalysisState == SymbolAnalysisState::JustCreated)
			symbol->AdvanceAnalysisState(SymbolAnalysisState::Ready);
	}

	for (const auto& symbol : membersList)
	{
		if (symbol->AnalysisState == SymbolAnalysisState::JustCreated)
			symbol->AdvanceAnalysisState(SymbolAnalysisState::Ready);
	}

	for (const auto& symbol : triviasList)
	{
		if (symbol->AnalysisState == SymbolAnalysisState::JustCreated)
			symbol->AdvanceAnalysisState(SymbolAnalysisState::Ready);
	}
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
