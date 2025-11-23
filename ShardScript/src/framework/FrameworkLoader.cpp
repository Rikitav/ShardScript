#include <shard/framework/FrameworkLoader.h>
#include <shard/framework/FrameworkModule.h>

#include <shard/parsing/LexicalAnalyzer.h>
#include <shard/parsing/SemanticAnalyzer.h>

#include <shard/parsing/semantic/SemanticModel.h>
#include <shard/parsing/semantic/SymbolTable.h>
#include <shard/parsing/analysis/DiagnosticsContext.h>
#include <shard/parsing/lexical/SyntaxTree.h>
#include <shard/parsing/reading/StringStreamReader.h>

#include <shard/runtime/GarbageCollector.h>
#include <shard/runtime/ConsoleHelper.h>
#include <shard/runtime/InboundVariablesContext.h>
#include <shard/runtime/ObjectInstance.h>

#include <shard/framework/primitives/BooleanPrimitive.h>
#include <shard/framework/primitives/IntegerPrimitive.h>
#include <shard/framework/primitives/CharPrimitive.h>
#include <shard/framework/primitives/StringPrimitive.h>
#include <shard/framework/primitives/ArrayPrimitive.h>

#include <shard/syntax/SyntaxKind.h>
#include <shard/syntax/SymbolAccesibility.h>

#include <shard/syntax/symbols/ClassSymbol.h>
#include <shard/syntax/symbols/StructSymbol.h>
#include <shard/syntax/symbols/MethodSymbol.h>
#include <shard/syntax/symbols/ParameterSymbol.h>

#include <shard/syntax/nodes/MemberDeclarationSyntax.h>
#include <shard/syntax/nodes/MemberDeclarations/MethodDeclarationSyntax.h>
#include <shard/syntax/nodes/MemberDeclarations/NamespaceDeclarationSyntax.h>
#include <shard/syntax/nodes/MemberDeclarations/ClassDeclarationSyntax.h>

#include <iostream>
#include <string>
#include <stdexcept>
#include <vector>

#include "filesystem/File.cpp"

using namespace shard::syntax::nodes;
using namespace shard::parsing::analysis;
using namespace shard::parsing::lexical;

using namespace shard::framework;
using namespace shard::runtime;
using namespace shard::syntax;
using namespace shard::syntax::symbols;
using namespace shard::parsing;
using namespace shard::parsing::semantic;

static ObjectInstance* Gc_Info(InboundVariablesContext* arguments)
{
	std::wcout << "Garbage collector info dump" << std::endl;
	for (ObjectInstance* reg : GarbageCollector::Heap)
	{
		std::wcout
			<< L" * " << reg->Ptr
			<< L" : " << reg->Id
			<< L" : " << reg->Info->Name
			<< L" : " << reg->ReferencesCounter << std::endl;
	}

	return nullptr; // void
}

static ObjectInstance* Print(InboundVariablesContext* arguments)
{
	ObjectInstance* instance = arguments->Variables.at(L"message");
	ConsoleHelper::Write(instance);
	return nullptr; // void
}

static ObjectInstance* Println(InboundVariablesContext* arguments)
{
	ObjectInstance* instance = arguments->Variables.at(L"message");
	ConsoleHelper::WriteLine(instance);
	return nullptr; // void
}

static ObjectInstance* Input(InboundVariablesContext* arguments)
{
	std::wstring input;
	getline(std::wcin, input);
	return ObjectInstance::FromValue(input);
}

static ObjectInstance* Impl_typeof(InboundVariablesContext* arguments)
{
	ObjectInstance* instance = arguments->Variables.at(L"object");
	if (instance == GarbageCollector::NullInstance)
		throw std::runtime_error("cannot get type of null instance");

	return ObjectInstance::FromValue(instance->Info->Name);
}

static ObjectInstance* Impl_sizeof(InboundVariablesContext* arguments)
{
	ObjectInstance* instance = arguments->Variables.at(L"object");
	if (instance == GarbageCollector::NullInstance)
		throw std::runtime_error("cannot get size of null instance");

	return ObjectInstance::FromValue(static_cast<int>(instance->Info->MemoryBytesSize));
}

std::vector<FrameworkModule*> FrameworkLoader::Modules = {
	new FileSystem_File()
};

void FrameworkLoader::Load(SemanticModel& semanticModel, DiagnosticsContext& diagnostics)
{
	ResolvePrmitives(semanticModel);
	ResolveGlobalMethods(semanticModel);

	for (FrameworkModule* module : Modules)
	{
		LoadSingleModule(module, semanticModel, diagnostics);
		delete module;
	}

	Modules.clear();
}

void FrameworkLoader::LoadSingleModule(FrameworkModule* module, SemanticModel& semanticModel, DiagnosticsContext& diagnostics)
{
	SyntaxTree tree;
	StringStreamReader reader = StringStreamReader(module->GetSourceCode());
	LexicalAnalyzer lexer = LexicalAnalyzer(diagnostics);
	SemanticAnalyzer semanter = SemanticAnalyzer(diagnostics);

	lexer.FromSourceReader(tree, reader);
	semanter.Analyze(tree, semanticModel);

	NamespaceDeclarationSyntax* fileNamespace = static_cast<NamespaceDeclarationSyntax*>(tree.CompilationUnits.at(0)->Members.at(0));
	ClassDeclarationSyntax* fileClass = static_cast<ClassDeclarationSyntax*>(fileNamespace->Members.at(0));

	for (MemberDeclarationSyntax* member : fileClass->Members)
	{
		switch (member->Kind)
		{
			case SyntaxKind::MethodDeclaration:
			{
				MethodDeclarationSyntax* method = static_cast<MethodDeclarationSyntax*>(member);
				MethodSymbol* symbol = static_cast<MethodSymbol*>(semanticModel.Table->LookupSymbol(method));
				if (!module->BindMethod(symbol))
				{
					diagnostics.ReportError(method->IdentifierToken, L"Unexpected method in System.File class loader");
				}

				break;
			}
		}
	}
}

void FrameworkLoader::ResolvePrmitives(SemanticModel& semanticModel)
{
	SymbolTable::Primitives::Void = new StructSymbol(L"Void");
	SymbolTable::Primitives::Any = new StructSymbol(L"Any");

	SymbolTable::Primitives::Boolean = new StructSymbol(L"Boolean");
	SymbolTable::Primitives::Integer = new StructSymbol(L"Integer");
	SymbolTable::Primitives::Char = new StructSymbol(L"Char");
	SymbolTable::Primitives::String = new ClassSymbol(L"String");
	SymbolTable::Primitives::Array = new ClassSymbol(L"Array");

	SymbolTable::Primitives::Void->MemoryBytesSize = 0;
	SymbolTable::Primitives::Any->MemoryBytesSize = 0;

	SymbolTable::Primitives::Boolean->MemoryBytesSize = sizeof(bool);
	SymbolTable::Primitives::Integer->MemoryBytesSize = sizeof(int);
	SymbolTable::Primitives::Char->MemoryBytesSize = sizeof(wchar_t);
	SymbolTable::Primitives::String->MemoryBytesSize = sizeof(std::wstring);
	SymbolTable::Primitives::Array->MemoryBytesSize = sizeof(int);

	//semanticModel.Table->GlobalScope->DeclareSymbol(SymbolTable::Primitives::Boolean);
	//semanticModel.Table->GlobalScope->DeclareSymbol(SymbolTable::Primitives::Integer);
	//semanticModel.Table->GlobalScope->DeclareSymbol(SymbolTable::Primitives::Char);
	//semanticModel.Table->GlobalScope->DeclareSymbol(SymbolTable::Primitives::String);

	BooleanPrimitive::Reflect(SymbolTable::Primitives::Boolean);
	IntegerPrimitive::Reflect(SymbolTable::Primitives::Integer);
	CharPrimitive::Reflect(SymbolTable::Primitives::Char);
	StringPrimitive::Reflect(SymbolTable::Primitives::String);
	ArrayPrimitive::Reflect(SymbolTable::Primitives::Array);
}

void FrameworkLoader::ResolveGlobalMethods(SemanticModel& semanticModel)
{
	// gc_info
	{
		MethodSymbol* gcInfoMethod = new MethodSymbol(L"gc_info", Gc_Info);
		gcInfoMethod->ReturnType = SymbolTable::Primitives::Void;
		gcInfoMethod->Accesibility = SymbolAccesibility::Public;
		gcInfoMethod->IsStatic = true;
		
		semanticModel.Table->GlobalType->Methods.push_back(gcInfoMethod);
	}

	// print
	{
		MethodSymbol* printMethod = new MethodSymbol(L"print", Print);
		printMethod->ReturnType = SymbolTable::Primitives::Void;
		printMethod->Accesibility = SymbolAccesibility::Public;
		printMethod->IsStatic = true;

		ParameterSymbol* printMessageParam = new ParameterSymbol(L"message");
		printMessageParam->Type = SymbolTable::Primitives::String;
		printMethod->Parameters.push_back(printMessageParam);
		
		semanticModel.Table->GlobalType->Methods.push_back(printMethod);
	}

	// println
	{
		MethodSymbol* printlnMethod = new MethodSymbol(L"println", Println);
		printlnMethod->ReturnType = SymbolTable::Primitives::Void;
		printlnMethod->Accesibility = SymbolAccesibility::Public;
		printlnMethod->IsStatic = true;

		ParameterSymbol* printlnMessageParam = new ParameterSymbol(L"message");
		printlnMessageParam->Type = SymbolTable::Primitives::String;
		printlnMethod->Parameters.push_back(printlnMessageParam);
		
		semanticModel.Table->GlobalType->Methods.push_back(printlnMethod);
	}

	// typeof
	{
		MethodSymbol* typeofMethod = new MethodSymbol(L"typeof", Impl_typeof);
		typeofMethod->ReturnType = SymbolTable::Primitives::String;
		typeofMethod->Accesibility = SymbolAccesibility::Public;
		typeofMethod->IsStatic = true;

		ParameterSymbol* typeofObjectParam = new ParameterSymbol(L"object");
		typeofObjectParam->Type = SymbolTable::Primitives::Any;
		typeofMethod->Parameters.push_back(typeofObjectParam);

		semanticModel.Table->GlobalType->Methods.push_back(typeofMethod);
	}

	// sizeof
	{
		MethodSymbol* sizeofMethod = new MethodSymbol(L"sizeof", Impl_sizeof);
		sizeofMethod->ReturnType = SymbolTable::Primitives::Integer;
		sizeofMethod->Accesibility = SymbolAccesibility::Public;
		sizeofMethod->IsStatic = true;

		ParameterSymbol* typeofObjectParam = new ParameterSymbol(L"object");
		typeofObjectParam->Type = SymbolTable::Primitives::Any;
		sizeofMethod->Parameters.push_back(typeofObjectParam);

		semanticModel.Table->GlobalType->Methods.push_back(sizeofMethod);
	}
}
