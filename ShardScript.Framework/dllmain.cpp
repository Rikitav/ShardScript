#include <shard/runtime/framework/FrameworkLoader.h>

#include <shard/parsing/semantic/SymbolTable.h>

#include <shard/runtime/VirtualMachine.h>
#include <shard/runtime/GarbageCollector.h>
#include <shard/runtime/ConsoleHelper.h>
#include <shard/runtime/ArgumentsSpan.h>
#include <shard/runtime/ObjectInstance.h>
#include <shard/runtime/CallStackFrame.h>

#include <shard/syntax/SymbolAccesibility.h>

#include <shard/syntax/symbols/ClassSymbol.h>
#include <shard/syntax/symbols/StructSymbol.h>
#include <shard/syntax/symbols/MethodSymbol.h>
#include <shard/syntax/symbols/ParameterSymbol.h>
#include <shard/syntax/symbols/TypeSymbol.h>

#include <windows.h>
#include <iostream>
#include <string>
#include <stdexcept>
#include <vector>
#include <cstdint>

#include "primitives/PrimitivesLoading.h"
#include "system/filesystem/File.cpp"
#include "system/filesystem/Directory.cpp"
#include "system/collections/List.cpp"
#include "system/Random.cpp"

using namespace shard;

static ObjectInstance* Gc_Info(const VirtualMachine* host, const MethodSymbol* method, ArgumentsSpan& arguments)
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

static ObjectInstance* Var_Info(const VirtualMachine* host, const MethodSymbol* method, ArgumentsSpan& arguments)
{
	std::wcout << "\nCall stack frame variables dump :" << std::endl;
	
	CallStackFrame* frame = host->CurrentFrame()->PreviousFrame;
	for (int i = 0; i < method->EvalStackLocalsCount; i++)
	{
		ObjectInstance* reg = frame->EvalStack[i];
		std::wcout
			<< L" * | ID : " << reg->Id
			<< L" | TYPE : '" << reg->Info->Name << "'"
			<< L" | REFS : " << reg->ReferencesCounter
			<< L" | PTR : " << reg->Ptr << std::endl;
	}

	std::wcout << "Total count : " << method->EvalStackLocalsCount << std::endl;
	return nullptr; // void
}

static ObjectInstance* Print(const VirtualMachine* host, const MethodSymbol* method, ArgumentsSpan& arguments)
{
	ObjectInstance* instance = arguments[0]; // var
	TypeSymbol* type = const_cast<TypeSymbol*>(instance->Info);

	if (type->IsPrimitive())
	{
		ConsoleHelper::Write(instance);
		return nullptr; // void
	}

	static std::wstring methodWName = L"ToString";
	MethodSymbol* toString = type->FindMethod(methodWName, std::vector<TypeSymbol*>());

	if (toString != nullptr)
	{
		host->InvokeMethod(toString, { instance });
		ObjectInstance* result = host->CurrentFrame()->PopStack();

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

static ObjectInstance* Println(const VirtualMachine* host, const MethodSymbol* method, ArgumentsSpan& arguments)
{
	ObjectInstance* instance = arguments[0]; // var
	TypeSymbol* type = const_cast<TypeSymbol*>(instance->Info);

	if (type->IsPrimitive())
	{
		ConsoleHelper::WriteLine(instance);
		return nullptr; // void
	}

	static std::wstring methodWName = L"ToString";
	MethodSymbol* toString = type->FindMethod(methodWName, std::vector<TypeSymbol*>());

	if (toString != nullptr)
	{
		host->InvokeMethod(toString, { instance });
		ObjectInstance* result = host->CurrentFrame()->PopStack();

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

static ObjectInstance* Input(const VirtualMachine* host, const MethodSymbol* method, ArgumentsSpan& arguments)
{
	std::wstring input;
	getline(std::wcin, input);
	return ObjectInstance::FromValue(input);
}

static ObjectInstance* Impl_typeof(const VirtualMachine* host, const MethodSymbol* method, ArgumentsSpan& arguments)
{
	const ObjectInstance* instance = arguments[0];
	if (instance == GarbageCollector::NullInstance)
		throw std::runtime_error("cannot get type of null instance");

	return ObjectInstance::FromValue(instance->Info->Name);
}

static ObjectInstance* Impl_sizeof(const VirtualMachine* host, const MethodSymbol* method, ArgumentsSpan& arguments)
{
	const ObjectInstance* instance = arguments[0];
	if (instance == GarbageCollector::NullInstance)
		throw std::runtime_error("cannot get size of null instance");

	return ObjectInstance::FromValue(static_cast<int64_t>(instance->Info->MemoryBytesSize));
}

static void ResolvePrimitives()
{
	SymbolTable::Primitives::Void = new StructSymbol(L"Void");
	SymbolTable::Primitives::Any = new StructSymbol(L"Any");

	SymbolTable::Primitives::Boolean = new StructSymbol(L"Boolean");
	SymbolTable::Primitives::Integer = new StructSymbol(L"Integer");
	SymbolTable::Primitives::Double = new StructSymbol(L"Double");
	SymbolTable::Primitives::Char = new StructSymbol(L"Char");
	SymbolTable::Primitives::String = new ClassSymbol(L"String");
	SymbolTable::Primitives::Array = new ClassSymbol(L"Array");

	SymbolTable::Primitives::Void->MemoryBytesSize = 0;
	SymbolTable::Primitives::Any->MemoryBytesSize = 0;

	SymbolTable::Primitives::Boolean->MemoryBytesSize = sizeof(bool);
	SymbolTable::Primitives::Integer->MemoryBytesSize = sizeof(int64_t);
	SymbolTable::Primitives::Double->MemoryBytesSize = sizeof(double);
	SymbolTable::Primitives::Char->MemoryBytesSize = sizeof(wchar_t);
	SymbolTable::Primitives::String->MemoryBytesSize = sizeof(std::wstring);
	SymbolTable::Primitives::Array->MemoryBytesSize = sizeof(int); // _length field

	BooleanPrimitive::Reflect(SymbolTable::Primitives::Boolean);
	IntegerPrimitive::Reflect(SymbolTable::Primitives::Integer);
	DoublePrimitive::Reflect(SymbolTable::Primitives::Double);
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

			shard::FrameworkLoader::AddModule(new FileSystem_Directory());
			shard::FrameworkLoader::AddModule(new FileSystem_File());
			shard::FrameworkLoader::AddModule(new Collections_List());
			shard::FrameworkLoader::AddModule(new Random());
			break;
        }

        case DLL_THREAD_ATTACH:
        case DLL_THREAD_DETACH:
        case DLL_PROCESS_DETACH:
            break;
    }

    return TRUE;
}
