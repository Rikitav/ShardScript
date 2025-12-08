#include <framework/PrimitivesLoading.h>

#include <shard/framework/FrameworkLoader.h>

#include <shard/parsing/semantic/SemanticModel.h>
#include <shard/parsing/semantic/SymbolTable.h>
#include <shard/parsing/analysis/DiagnosticsContext.h>
#include <shard/parsing/lexical/SyntaxTree.h>
#include <shard/parsing/reading/StringStreamReader.h>

#include <shard/runtime/AbstractInterpreter.h>
#include <shard/runtime/GarbageCollector.h>
#include <shard/runtime/ConsoleHelper.h>
#include <shard/runtime/InboundVariablesContext.h>
#include <shard/runtime/ObjectInstance.h>

#include <shard/syntax/SyntaxKind.h>
#include <shard/syntax/SymbolAccesibility.h>

#include <shard/syntax/symbols/ClassSymbol.h>
#include <shard/syntax/symbols/StructSymbol.h>
#include <shard/syntax/symbols/MethodSymbol.h>
#include <shard/syntax/symbols/ParameterSymbol.h>
#include <shard/syntax/symbols/TypeSymbol.h>

#include <shard/syntax/nodes/MemberDeclarationSyntax.h>
#include <shard/syntax/nodes/ArgumentsListSyntax.h>
#include <shard/syntax/nodes/CompilationUnitSyntax.h>

#include <shard/syntax/nodes/MemberDeclarations/MethodDeclarationSyntax.h>
#include <shard/syntax/nodes/MemberDeclarations/NamespaceDeclarationSyntax.h>
#include <shard/syntax/nodes/MemberDeclarations/ClassDeclarationSyntax.h>
#include <shard/syntax/nodes/MemberDeclarations/StructDeclarationSyntax.h>

#include <windows.h>
#include <iostream>
#include <string>
#include <stdexcept>
#include <vector>

#include "src/filesystem/File.cpp"
#include "src/filesystem/Directory.cpp"
#include "src/collections/List.cpp"

using namespace shard::framework;
using namespace shard::runtime;

using namespace shard::syntax;
using namespace shard::syntax::symbols;
using namespace shard::syntax::nodes;

using namespace shard::parsing;
using namespace shard::parsing::analysis;
using namespace shard::parsing::lexical;
using namespace shard::parsing::semantic;

static ObjectInstance* Gc_Info(const MethodSymbol* symbol, InboundVariablesContext* arguments)
{
	std::wcout << "\nGarbage collector info dump" << std::endl;
	for (ObjectInstance* reg : GarbageCollector::Heap)
	{
		std::wcout
			<< L" * | ID : " << reg->Id
			<< L" | TYPE : '" << reg->Info->Name << "'"
			<< L" | REFS : " << reg->ReferencesCounter
			<< L" | PTR : " << reg->Ptr << std::endl;
	}

	std::wcout << "Total count : " << GarbageCollector::Heap.size() << std::endl;
	return nullptr; // void
}

static ObjectInstance* Var_Info(const MethodSymbol* symbol, InboundVariablesContext* arguments)
{
	int count = 0;
	std::wcout << "\nCall stack frame variables dump" << std::endl;
	for (const InboundVariablesContext* context = AbstractInterpreter::CurrentFrame()->PreviousFrame->VariablesStack.top(); context != nullptr; context = context->Previous)
	{
		for (const auto& varReg : context->Variables)
		{
			count += 1;
			ObjectInstance* reg = varReg.second;
			std::wcout
				<< L" * | ID : " << reg->Id
				<< L" | TYPE : '" << reg->Info->Name << "'"
				<< L" | REFS : " << reg->ReferencesCounter
				<< L" | PTR : " << reg->Ptr << std::endl;
		}
	}

	std::wcout << "Total count : " << count << std::endl;
	return nullptr; // void
}

static ObjectInstance* Print(const MethodSymbol* symbol, InboundVariablesContext* arguments)
{
	ObjectInstance* instance = arguments->Variables.at(L"message"); // var
	TypeSymbol* type = const_cast<TypeSymbol*>(instance->Info);

	if (type->IsPrimitive())
	{
		ConsoleHelper::Write(instance);
		return nullptr; // void
	}

	std::wstring methodWName = L"ToString";
	MethodSymbol* toString = type->FindMethod(methodWName, std::vector<TypeSymbol*>());
	if (toString != nullptr)
	{
		InboundVariablesContext* toStringArgs = AbstractInterpreter::CreateArgumentsContext(std::vector<ArgumentSyntax*>(), std::vector<ParameterSymbol*>(), toString->IsStatic, instance);
		ObjectInstance* result = AbstractInterpreter::ExecuteMethod(symbol, instance->Info, toStringArgs);
		if (type != SymbolTable::Primitives::String)
		{
#pragma warning (push)
#pragma warning (disable: 4244)
			std::string methodName = std::string(toString->FullName.begin(), toString->FullName.end());
			throw std::runtime_error("Failed to evaluate ToString method of \'" + methodName + "\'. Reason: returned not a string!");
#pragma warning (pop)
		}

		ConsoleHelper::Write(instance);
		return nullptr; // void
	}

	ConsoleHelper::Write(type->FullName);
	return nullptr; // void
}

static ObjectInstance* Println(const MethodSymbol* symbol, InboundVariablesContext* arguments)
{
	ObjectInstance* instance = arguments->Variables.at(L"message");
	TypeSymbol* type = const_cast<TypeSymbol*>(instance->Info);

	if (type->IsPrimitive())
	{
		ConsoleHelper::WriteLine(instance);
		return nullptr; // void
	}

	std::wstring methodWName = L"ToString";
	MethodSymbol* toString = type->FindMethod(methodWName, std::vector<TypeSymbol*>());
	if (toString != nullptr)
	{
		InboundVariablesContext* toStringArgs = AbstractInterpreter::CreateArgumentsContext(std::vector<ArgumentSyntax*>(), std::vector<ParameterSymbol*>(), toString->IsStatic, instance);
		ObjectInstance* result = AbstractInterpreter::ExecuteMethod(toString, instance->Info, toStringArgs);
		if (result->Info != SymbolTable::Primitives::String)
		{
			std::string methodName = std::string(toString->FullName.begin(), toString->FullName.end());
			throw std::runtime_error("Failed to evaluate ToString method of \'" + methodName + "\'. Reason: returned not a string!");
		}

		ConsoleHelper::WriteLine(result);
		return nullptr; // void
	}

	ConsoleHelper::WriteLine(type->FullName);
	return nullptr; // void
}

static ObjectInstance* Input(const MethodSymbol* symbol, InboundVariablesContext* arguments)
{
	std::wstring input;
	getline(std::wcin, input);
	return ObjectInstance::FromValue(input);
}

static ObjectInstance* Impl_typeof(const MethodSymbol* symbol, InboundVariablesContext* arguments)
{
	ObjectInstance* instance = arguments->Variables.at(L"object");
	if (instance == GarbageCollector::NullInstance)
		throw std::runtime_error("cannot get type of null instance");

	return ObjectInstance::FromValue(instance->Info->Name);
}

static ObjectInstance* Impl_sizeof(const MethodSymbol* symbol, InboundVariablesContext* arguments)
{
	ObjectInstance* instance = arguments->Variables.at(L"object");
	if (instance == GarbageCollector::NullInstance)
		throw std::runtime_error("cannot get size of null instance");

	return ObjectInstance::FromValue(static_cast<int>(instance->Info->MemoryBytesSize));
}

static void ResolvePrimitives()
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
	SymbolTable::Primitives::Array->MemoryBytesSize = sizeof(int); // _length field

	BooleanPrimitive::Reflect(SymbolTable::Primitives::Boolean);
	IntegerPrimitive::Reflect(SymbolTable::Primitives::Integer);
	CharPrimitive::Reflect(SymbolTable::Primitives::Char);
	StringPrimitive::Reflect(SymbolTable::Primitives::String);
	ArrayPrimitive::Reflect(SymbolTable::Primitives::Array);
}

static void ResolveGlobalMethods()
{
	// gc_info
	{
		MethodSymbol* gcInfoMethod = new MethodSymbol(L"gc_info", Gc_Info);
		gcInfoMethod->ReturnType = SymbolTable::Primitives::Void;
		gcInfoMethod->Accesibility = SymbolAccesibility::Public;
		gcInfoMethod->IsStatic = true;

		SymbolTable::Global::Type->Methods.push_back(gcInfoMethod);
	}

	// Var_Info
	{
		MethodSymbol* gcInfoMethod = new MethodSymbol(L"var_info", Var_Info);
		gcInfoMethod->ReturnType = SymbolTable::Primitives::Void;
		gcInfoMethod->Accesibility = SymbolAccesibility::Public;
		gcInfoMethod->IsStatic = true;

		SymbolTable::Global::Type->Methods.push_back(gcInfoMethod);
	}

	// print
	{
		MethodSymbol* printMethod = new MethodSymbol(L"print", Print);
		printMethod->ReturnType = SymbolTable::Primitives::Void;
		printMethod->Accesibility = SymbolAccesibility::Public;
		printMethod->IsStatic = true;

		ParameterSymbol* printMessageParam = new ParameterSymbol(L"message");
		printMessageParam->Type = SymbolTable::Primitives::Any;
		printMethod->Parameters.push_back(printMessageParam);

		SymbolTable::Global::Type->Methods.push_back(printMethod);
	}

	// println
	{
		MethodSymbol* printlnMethod = new MethodSymbol(L"println", Println);
		printlnMethod->ReturnType = SymbolTable::Primitives::Void;
		printlnMethod->Accesibility = SymbolAccesibility::Public;
		printlnMethod->IsStatic = true;

		ParameterSymbol* printlnMessageParam = new ParameterSymbol(L"message");
		printlnMessageParam->Type = SymbolTable::Primitives::Any;
		printlnMethod->Parameters.push_back(printlnMessageParam);

		SymbolTable::Global::Type->Methods.push_back(printlnMethod);
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

		SymbolTable::Global::Type->Methods.push_back(typeofMethod);
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

		SymbolTable::Global::Type->Methods.push_back(sizeofMethod);
	}
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    switch (ul_reason_for_call)
    {
        case DLL_PROCESS_ATTACH:
        {
			ResolvePrimitives();
			ResolveGlobalMethods();

			shard::framework::FrameworkLoader::AddModule(new FileSystem_Directory());
			shard::framework::FrameworkLoader::AddModule(new FileSystem_File());
			shard::framework::FrameworkLoader::AddModule(new Collections_List());
			break;
        }

        case DLL_THREAD_ATTACH:
        case DLL_THREAD_DETACH:
        case DLL_PROCESS_DETACH:
            break;
    }

    return TRUE;
}
