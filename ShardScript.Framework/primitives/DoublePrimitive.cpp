#include <shard/runtime/ArgumentsSpan.hpp>
#include <shard/runtime/ObjectInstance.hpp>
#include <shard/parsing/semantic/SymbolTable.hpp>

#include <shard/syntax/SyntaxSymbol.hpp>
#include <shard/syntax/SymbolAccesibility.hpp>
#include <shard/syntax/symbols/TypeSymbol.hpp>
#include <shard/syntax/symbols/MethodSymbol.hpp>

#include <new>
#include <vector>
#include <string>

#include "PrimitivesLoading.hpp"

using namespace shard;

// Integer methods
static ObjectInstance* ToString(const VirtualMachine* host, const MethodSymbol* method, ArgumentsSpan& arguments)
{
	const ObjectInstance* instance = arguments[0];
	double value = instance->AsDouble();
	std::wstring str = std::to_wstring(value);
	return ObjectInstance::FromValue(str);
}

void DoublePrimitive::Reflect(TypeSymbol* symbol)
{
	// ToString()
	MethodSymbol* toString = new MethodSymbol(L"ToString", ToString);
	toString->Accesibility = SymbolAccesibility::Public;
	toString->ReturnType = SymbolTable::Primitives::String;
	toString->IsStatic = false;
	symbol->Methods.push_back(toString);
}
