#include <shard/runtime/ArgumentsSpan.hpp>
#include <shard/runtime/ObjectInstance.hpp>

#include <shard/parsing/semantic/SymbolTable.hpp>

#include <shard/syntax/SymbolAccesibility.hpp>
#include <shard/syntax/symbols/TypeSymbol.hpp>
#include <shard/syntax/symbols/MethodSymbol.hpp>

#include <string>

#include "PrimitivesLoading.hpp"

using namespace shard;

// Boolean methods
static ObjectInstance* ToString(const VirtualMachine* host, const MethodSymbol* method, ArgumentsSpan& arguments)
{
	ObjectInstance* instance = arguments[0]; // this
	bool value = instance->AsBoolean();
	std::wstring str = value ? L"true" : L"false";
	return ObjectInstance::FromValue(str);
}

void BooleanPrimitive::Reflect(TypeSymbol* symbol)
{
	// ToString()
	MethodSymbol* toString = new MethodSymbol(L"ToString", ToString);
	toString->Accesibility = SymbolAccesibility::Public;
	toString->ReturnType = SymbolTable::Primitives::String;
	toString->IsStatic = false;
	symbol->Methods.push_back(toString);
}
