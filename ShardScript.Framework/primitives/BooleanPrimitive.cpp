#include <shard/runtime/ArgumentsSpan.h>
#include <shard/runtime/ObjectInstance.h>

#include <shard/parsing/semantic/SymbolTable.h>

#include <shard/syntax/SymbolAccesibility.h>
#include <shard/syntax/symbols/TypeSymbol.h>
#include <shard/syntax/symbols/MethodSymbol.h>

#include <string>

#include "PrimitivesLoading.h"

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
