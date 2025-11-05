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

#include <shard/runtime/GarbageCollector.h>
#include <shard/runtime/ConsoleHelper.h>

#include <iostream>
#include <vector>
#include <ranges>
#include <string>

using namespace std;
using namespace std::ranges;
using namespace std::views;
using namespace shard::runtime;
using namespace shard::syntax;
using namespace shard::syntax::symbols;
using namespace shard::parsing::semantic;

ObjectInstance* Gc_Info(InboundVariablesContext* arguments)
{
	wcout << "Garbage collector info dump" << endl;
	for (ObjectInstance* reg : GarbageCollector::Heap)
	{
		wcout
			<< L" * " << reg->Ptr
			<< L" : " << reg->Id
			<< L" : " << reg->Info->Name
			<< L" : " << reg->ReferencesCounter << endl;
	}

	return nullptr;
}

ObjectInstance* Print(InboundVariablesContext* arguments)
{
	ObjectInstance* instance = arguments->Variables.at(L"message");
	ConsoleHelper::Write(instance);
	return nullptr;
}

ObjectInstance* Println(InboundVariablesContext* arguments)
{
	ObjectInstance* instance = arguments->Variables.at(L"message");
	ConsoleHelper::WriteLine(instance);
	return nullptr;
}

void SymbolTable::ResolvePrmitives()
{
	Primitives::Void = new StructSymbol(L"Void");
	Primitives::Boolean = new StructSymbol(L"Boolean");
	Primitives::Integer = new StructSymbol(L"Integer");
	Primitives::Char = new StructSymbol(L"Char");
	Primitives::String = new ClassSymbol(L"String");

	Primitives::Void->MemoryBytesSize = 0;
	Primitives::Boolean->MemoryBytesSize = sizeof(bool);
	Primitives::Integer->MemoryBytesSize = sizeof(int);
	Primitives::Char->MemoryBytesSize = sizeof(wchar_t);
	Primitives::String->MemoryBytesSize = sizeof(wstring);

	GlobalScope->DeclareSymbol(Primitives::Boolean);
	GlobalScope->DeclareSymbol(Primitives::Integer);
	GlobalScope->DeclareSymbol(Primitives::Char);
	GlobalScope->DeclareSymbol(Primitives::String);

	BooleanPrimitive::Reflect(Primitives::Boolean);
	IntegerPrimitive::Reflect(Primitives::Integer);
	CharPrimitive::Reflect(Primitives::Char);
	StringPrimitive::Reflect(Primitives::String);

	GlobalType = new TypeSymbol(GlobalTypeName, SyntaxKind::CompilationUnit);
}

void SymbolTable::ResolveGlobalMethods()
{
	MethodSymbol* gcInfoMethod = new MethodSymbol(L"gc_info", Gc_Info);
	gcInfoMethod->ReturnType = SymbolTable::Primitives::Void;
	gcInfoMethod->Accesibility = SymbolAccesibility::Public;
	gcInfoMethod->IsStatic = true;

	MethodSymbol* printMethod = new MethodSymbol(L"print", Print);
	printMethod->ReturnType = SymbolTable::Primitives::Void;
	printMethod->Accesibility = SymbolAccesibility::Public;
	printMethod->IsStatic = true;

	ParameterSymbol* printMessageParam = new ParameterSymbol(L"message");
	printMessageParam->Type = SymbolTable::Primitives::String;
	printMethod->Parameters.push_back(printMessageParam);

	MethodSymbol* printlnMethod = new MethodSymbol(L"println", Println);
	printlnMethod->ReturnType = SymbolTable::Primitives::Void;
	printlnMethod->Accesibility = SymbolAccesibility::Public;
	printlnMethod->IsStatic = true;

	ParameterSymbol* printlnMessageParam = new ParameterSymbol(L"message");
	printlnMessageParam->Type = SymbolTable::Primitives::String;
	printlnMethod->Parameters.push_back(printlnMessageParam);

	GlobalType->Methods.push_back(gcInfoMethod);
	GlobalType->Methods.push_back(printMethod);
	GlobalType->Methods.push_back(printlnMethod);
}

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
	auto cond = [](const SyntaxSymbol* symbol) { return symbol->Kind == SyntaxKind::ClassDeclaration || symbol->Kind == SyntaxKind::StructDeclaration; };
	auto trans = [](const SyntaxSymbol* symbol) { return static_cast<TypeSymbol*>((SyntaxSymbol*)symbol); };
	auto types = symbolToNodeMap | keys | filter(cond) | transform(trans);
	return vector<TypeSymbol*>(types.begin(), types.end());
}
