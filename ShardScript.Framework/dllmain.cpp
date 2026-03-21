#include <windows.h> // TODO: remove
#include <iostream>
#include <string>
#include <stdexcept>
#include <vector>
#include <cstdint>

#include <ShardScript.hpp>
#include <primitives/PrimitivesLoading.hpp>
#include <system/filesystem/File.cpp>
#include <system/filesystem/Directory.cpp>
#include <system/collections/List.cpp>
#include <system/Random.cpp>

#define ONLY_ONCE static bool visited = false; if (visited) return; visited = true;
#define STATIC true

using namespace shard;

static ObjectInstance* Impl_Gc_Info(const CallState& context)
{
	std::wcout << "\nGarbage collector info dump" << std::endl;
	for (ObjectInstance* reg : context.Collector.Heap)
	{
		std::wcout
			<< L" * PTR : " << reg->Memory
			<< L" | TYPE : '" << reg->Info->Name << "'"
			<< L" | REFS : " << reg->ReferencesCounter << std::endl;
	}

	std::wcout << "Total count : " << context.Collector.Heap.size() << std::endl;
	return nullptr; // void
}

static ObjectInstance* Impl_Var_Info(const CallState& context)
{
	std::wcout << "\nCall stack frame variables dump :" << std::endl;
	
	CallStackFrame* frame = context.Runtimer.CurrentFrame()->PreviousFrame;
	for (int i = 0; i < context.Method->EvalStackLocalsCount; i++)
	{
		ObjectInstance* reg = frame->EvalStack[i];
		std::wcout
			<< L" * PTR : " << reg->Memory
			<< L" | TYPE : '" << reg->Info->Name << "'"
			<< L" | REFS : " << reg->ReferencesCounter << std::endl;
	}

	std::wcout << "Total count : " << context.Method->EvalStackLocalsCount << std::endl;
	return nullptr; // void
}

static ObjectInstance* Impl_Print(const CallState& context)
{
	ObjectInstance* instance = context.Args[0]; // var
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
		context.Runtimer.InvokeMethod(toString, { instance });
		ObjectInstance* result = context.Runtimer.CurrentFrame()->PopStack();

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

static ObjectInstance* Impl_Println(const CallState& context)
{
	ObjectInstance* instance = context.Args[0]; // var
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
		context.Runtimer.InvokeMethod(toString, { instance });
		ObjectInstance* result = context.Runtimer.CurrentFrame()->PopStack();

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

static ObjectInstance* Impl_Input(const CallState& context)
{
	std::wstring input;
	getline(std::wcin, input);
	return context.Collector.FromValue(input);
}

static ObjectInstance* Impl_typeof(const CallState& context)
{
	const ObjectInstance* instance = context.Args[0];
	if (instance == GarbageCollector::NullInstance)
		throw std::runtime_error("cannot get type of null instance");

	return context.Collector.FromValue(instance->Info->Name);
}

static ObjectInstance* Impl_sizeof(const CallState& context)
{
	const ObjectInstance* instance = context.Args[0];
	if (instance == GarbageCollector::NullInstance)
		throw std::runtime_error("cannot get size of null instance");

	return context.Collector.FromValue(static_cast<int64_t>(instance->Info->MemoryBytesSize));
}

static void ReflectGlobalMethods(CompilationContext& context)
{
	// gc_info
	MethodSymbol* gcInfoMethod = SymbolFactory::Method(ACS_PUBLIC, STATIC, TYPE_VOID, L"gc_info", Impl_Gc_Info);
	context.GetSemanticAnalyzer().AddSymbol(gcInfoMethod);

	// Var_Info
	MethodSymbol* varInfoMethod = SymbolFactory::Method(ACS_PUBLIC, STATIC, TYPE_VOID, L"var_info", Impl_Var_Info);
	context.GetSemanticAnalyzer().AddSymbol(varInfoMethod);

	// inputln
	MethodSymbol* inputlnMethod = SymbolFactory::Method(ACS_PUBLIC, STATIC, TYPE_STRING, L"inputln", Impl_Input);
	context.GetSemanticAnalyzer().AddSymbol(inputlnMethod);

	// print
	MethodSymbol* printMethod = SymbolFactory::Method(ACS_PUBLIC, STATIC, TYPE_VOID, L"print", Impl_Print);
	printMethod->Parameters.push_back(SymbolFactory::Parameter(L"message", TYPE_ANY));
	context.GetSemanticAnalyzer().AddSymbol(printMethod);

	// println
	MethodSymbol* printlnMethod = SymbolFactory::Method(ACS_PUBLIC, STATIC, TYPE_VOID, L"println", Impl_Println);
	printlnMethod->Parameters.push_back(SymbolFactory::Parameter(L"object", TYPE_ANY));
	context.GetSemanticAnalyzer().AddSymbol(printlnMethod);

	// typeof
	MethodSymbol* typeofMethod = SymbolFactory::Method(ACS_PUBLIC, STATIC, TYPE_STRING, L"typeof", Impl_typeof);
	typeofMethod->Parameters.push_back(SymbolFactory::Parameter(L"object", TYPE_ANY));
	context.GetSemanticAnalyzer().AddSymbol(typeofMethod);

	// sizeof
	MethodSymbol* sizeofMethod = SymbolFactory::Method(ACS_PUBLIC, STATIC, TYPE_INT, L"sizeof", Impl_sizeof);
	sizeofMethod->Parameters.push_back(SymbolFactory::Parameter(L"object", TYPE_ANY));
	context.GetSemanticAnalyzer().AddSymbol(sizeofMethod);
}

static void ReflectPrimitives(CompilationContext& context)
{
	ONLY_ONCE

	BooleanPrimitive::Reflect(SymbolTable::Primitives::Boolean);
	IntegerPrimitive::Reflect(SymbolTable::Primitives::Integer);
	DoublePrimitive::Reflect(SymbolTable::Primitives::Double);
	CharPrimitive::Reflect(SymbolTable::Primitives::Char);
	StringPrimitive::Reflect(SymbolTable::Primitives::String);
	ArrayPrimitive::Reflect(SymbolTable::Primitives::Array);
}

static ShardLibMetadata LibMetadata = ShardLibMetadata
{
	.Name = L"ShardScript.Framework",
	.Description = L"ShardScript standart library framework",
	.Version = L"beta 0.6"
};

SHARDLIB_GETMETADATA
{
	memcpy(&LibMetadata, &lib, sizeof(ShardLibMetadata));
}

SHARDLIB_ENTRYPOINT
{
	ReflectPrimitives(context);
	ReflectGlobalMethods(context);
	
	context.AddModule(new FileSystem_Directory());
	context.AddModule(new FileSystem_File());
	context.AddModule(new Collections_List());
	context.AddModule(new Random());
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    switch (ul_reason_for_call)
    {
        case DLL_PROCESS_ATTACH:
        case DLL_THREAD_ATTACH:
        case DLL_THREAD_DETACH:
        case DLL_PROCESS_DETACH:
            break;
    }

    return TRUE;
}
