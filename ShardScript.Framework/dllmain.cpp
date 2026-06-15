#define ONLY_ONCE static bool visited = false; if (visited) return; visited = true;

#include <iostream>
#include <string>
#include <stdexcept>
#include <vector>
#include <cstdint>
#include <Windows.h>
#include <ShardScript.hpp>

constexpr auto STATIC = true;

using namespace shard;

typedef HMODULE(*CurrentModuleQuery)(void);

inline static HMODULE GetCurrentModule()
{
	HMODULE hModule = nullptr;
	CurrentModuleQuery query = GetCurrentModule;

	GetModuleHandleExW(
		GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS,
		reinterpret_cast<LPCWSTR>(query),
		&hModule);

	return hModule;
}

inline static void GetResource(const wchar_t* name, const wchar_t*& resourceData, std::size_t & resourceSize)
{
	HMODULE hModule = GetCurrentModule();
	HRSRC hResource = FindResourceW(hModule, name, L"SOURCE_CODE");
	if (!hResource)
		throw std::runtime_error("");

	HGLOBAL hResourceData = LoadResource(hModule, hResource);
	if (!hResourceData)
		throw std::runtime_error("");

	resourceSize = static_cast<std::size_t>(SizeofResource(hModule, hResource));
	resourceData = static_cast<wchar_t*>(LockResource(hResourceData));
	//FreeLibrary(hModule);
}

static ObjectInstance* Impl_Gc_Info(const CallState& context)
{
	std::wcout << "\nGarbage collector info dump" << std::endl;
	for (ObjectInstance* reg : context.Collector.Heap)
	{
		std::wcout
			<< L" * PTR : " << reg->getMemory()
			<< L" | TYPE : '" << reg->getInfo()->Name << "'"
			<< L" | REFS : " << reg->getReferencesCounter() << std::endl;
	}

	std::wcout << "Total count : " << context.Collector.Heap.size() << std::endl;
	return nullptr; // void
}

static ObjectInstance* Impl_Var_Info(const CallState& context)
{
	std::wcout << "\nCall stack frame variables dump :" << std::endl;

	CallStackFrame* frame = context.Runtimer.CurrentFrame()->PreviousFrame;
	for (int i = 0; i < context.Method->GetEvalStackLocalsCount(); i++)
	{
		ObjectInstance* reg = frame->EvalStack[i];
		std::wcout
			<< L" * PTR : " << reg->getMemory()
			<< L" | TYPE : '" << reg->getInfo()->Name << "'"
			<< L" | REFS : " << reg->getReferencesCounter() << std::endl;
	}

	std::wcout << "Total count : " << context.Method->GetEvalStackLocalsCount() << std::endl;
	return nullptr; // void
}

static ObjectInstance* Impl_typeof(const CallState& context)
{
	const ObjectInstance* instance = context.Args[0];
	if (instance == GarbageCollector::NullInstance)
		throw std::runtime_error("cannot get type of null instance");

	return context.Collector.FromValue(instance->getInfo()->Name);
}

static ObjectInstance* Impl_sizeof(const CallState& context)
{
	const ObjectInstance* instance = context.Args[0];
	if (instance == GarbageCollector::NullInstance)
		throw std::runtime_error("cannot get size of null instance");

	return context.Collector.FromValue(static_cast<int64_t>(instance->getInfo()->MemoryBytesSize));
}

static void ReflectGlobalMethods(CompilationContext& context)
{
	SymbolFactory factory(context.GetSemanticModel().Table.get());

	// gc_info
	MethodSymbol* gcInfoMethod = factory.Method(ACS_PUBLIC, STATIC, TYPE_VOID, L"gc_info", Impl_Gc_Info);
	context.GetSemanticAnalyzer().AddSymbol(gcInfoMethod);

	// Var_Info
	MethodSymbol* varInfoMethod = factory.Method(ACS_PUBLIC, STATIC, TYPE_VOID, L"var_info", Impl_Var_Info);
	context.GetSemanticAnalyzer().AddSymbol(varInfoMethod);

	// typeof
	MethodSymbol* typeofMethod = factory.Method(ACS_PUBLIC, STATIC, TYPE_STRING, L"typeof", Impl_typeof);
	typeofMethod->Parameters.push_back(factory.Parameter(L"object", TYPE_ANY));
	context.GetSemanticAnalyzer().AddSymbol(typeofMethod);

	// sizeof
	MethodSymbol* sizeofMethod = factory.Method(ACS_PUBLIC, STATIC, TYPE_INT, L"sizeof", Impl_sizeof);
	sizeofMethod->Parameters.push_back(factory.Parameter(L"object", TYPE_ANY));
	context.GetSemanticAnalyzer().AddSymbol(sizeofMethod);
}

static void ReflectPrimitives(CompilationContext& context)
{
	ONLY_ONCE

	//BooleanPrimitive::Reflect(SymbolTable::Primitives::Boolean);
	//IntegerPrimitive::Reflect(SymbolTable::Primitives::Integer);
	//DoublePrimitive::Reflect(SymbolTable::Primitives::Double);
	//CharPrimitive::Reflect(SymbolTable::Primitives::Char);
	//StringPrimitive::Reflect(SymbolTable::Primitives::String);
	//ArrayPrimitive::Reflect(SymbolTable::Primitives::Array);
}

static ShardLibMetadata LibMetadata = ShardLibMetadata
{
	.Name = L"ShardScript.Framework",
	.Description = L"ShardScript standart library framework",
	.Version = L"beta 0.6"
};

SHARDLIB_GETMETADATA
{
	memcpy(&lib, &LibMetadata, sizeof(ShardLibMetadata));
}

SHARDLIB_ENTRYPOINT
{
	ReflectPrimitives(context);
	ReflectGlobalMethods(context);

	const wchar_t* resourceData; std::size_t resourceSize;

	{
		GetResource(L"MATH_RANDOM", resourceData, resourceSize);
		auto reader = std::make_unique<StringStreamReader>(L"Random.ss", std::wstring(resourceData, resourceSize / sizeof(wchar_t)));
		context.ProvideSource(reader.get());
	}

	{
		GetResource(L"COLLECTIONS_LIST", resourceData, resourceSize);
		auto reader = std::make_unique<StringStreamReader>(L"List.ss", std::wstring(resourceData, resourceSize / sizeof(wchar_t)));
		context.ProvideSource(reader.get());
	}

	{
		GetResource(L"FILESYSTEM_FILE", resourceData, resourceSize);
		auto reader = std::make_unique<StringStreamReader>(L"File.ss", std::wstring(resourceData, resourceSize / sizeof(wchar_t)));
		context.ProvideSource(reader.get());
	}

	{
		GetResource(L"FILESYSTEM_DIRECTORY", resourceData, resourceSize);
		auto reader = std::make_unique<StringStreamReader>(L"Directory.ss", std::wstring(resourceData, resourceSize / sizeof(wchar_t)));
		context.ProvideSource(reader.get());
	}

	{
		GetResource(L"STDIO_CONSTREAM", resourceData, resourceSize);
		auto reader = std::make_unique<StringStreamReader>(L"Constream.ss", std::wstring(resourceData, resourceSize / sizeof(wchar_t)));
		context.ProvideSource(reader.get());
	}
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
