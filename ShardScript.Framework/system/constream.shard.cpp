#include <stdexcept>
#include <string>
#include <random>
#include <climits>
#include <cstdint>
#include <iostream>

#include <ShardScript.hpp>

using namespace shard;

static MethodSymbol* GetIPrintableToString()
{
	static std::wstring methodName = L"ToString";
	static MethodSymbol* method = TRAIT_PRINTABLE->FindMethod(methodName, std::vector<TypeSymbol*>());
	return method;
}

static std::string WStringToString(const std::wstring& value)
{
	std::string result;
	result.reserve(value.size());
	for (wchar_t ch : value)
		result.push_back(static_cast<char>(ch));
	return result;
}

static ObjectInstance* InvokeToString(const CallState& context, ObjectInstance* instance)
{
	TypeSymbol* type = const_cast<TypeSymbol*>(instance->getInfo());
	MethodSymbol* implementation = type->FindInterfaceImplementation(GetIPrintableToString());
	if (implementation == nullptr)
		throw std::runtime_error("Type '" + WStringToString(type->FullName) + "' does not implement IPrintable");

	context.Runtimer.InvokeMethod(implementation, { instance });
	ObjectInstance* result = context.Runtimer.CurrentFrame()->PopStack();
	if (result == nullptr || result->getInfo() != SymbolTable::Primitives::String)
		throw std::runtime_error("ToString did not return a string");

	return result;
}

static ObjectInstance* shard_constream_print(const CallState& context) noexcept(false)
{
	ObjectInstance* instance = context.Args[0];
	if (instance == nullptr || instance == context.Collector.NullInstance)
	{
		ConsoleHelper::Write(L"null");
		return nullptr;
	}

	ObjectInstance* result = InvokeToString(context, instance);
	ConsoleHelper::Write(result);
	return nullptr; // void
}

static ObjectInstance* shard_constream_println(const CallState& context) noexcept(false)
{
	ObjectInstance* instance = context.Args[0];
	if (instance == nullptr || instance == context.Collector.NullInstance)
	{
		ConsoleHelper::WriteLine(L"null");
		return nullptr;
	}

	ObjectInstance* result = InvokeToString(context, instance);
	ConsoleHelper::WriteLine(result);
	return nullptr; // void
}

static ObjectInstance* shard_constream_input(const CallState& context)
{
	std::wstring input;
	getline(std::wcin, input);
	return context.Collector.FromValue(input);
}

SHARDLIB_GETMETADATA
{
	lib.Name = L"shard.stdio";
	lib.Description = L"Console IO";
	lib.Version = L"1.0.0";
}

SHARDLIB_ENTRYPOINT
{
	SymbolBuilder<NamespaceSymbol> stdio(context, L"stdio");

	stdio.AddMethod(L"print", TYPE_VOID, LINK_STATIC, ACS_PUBLIC)
		 .AddParameter(L"message", TRAIT_PRINTABLE)
		 .SetCallback(&shard_constream_print);

	stdio.AddMethod(L"println", TYPE_VOID, LINK_STATIC, ACS_PUBLIC)
		 .AddParameter(L"message", TRAIT_PRINTABLE)
		 .SetCallback(&shard_constream_println);

	stdio.AddMethod(L"input", TYPE_STRING, LINK_STATIC, ACS_PUBLIC)
		 .SetCallback(&shard_constream_input);
}
