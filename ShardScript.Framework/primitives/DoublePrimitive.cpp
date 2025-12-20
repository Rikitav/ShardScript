#include <shard/runtime/InboundVariablesContext.h>
#include <shard/runtime/ObjectInstance.h>
#include <shard/parsing/semantic/SymbolTable.h>

#include <shard/syntax/SyntaxSymbol.h>
#include <shard/syntax/SymbolAccesibility.h>
#include <shard/syntax/symbols/TypeSymbol.h>
#include <shard/syntax/symbols/MethodSymbol.h>
#include <shard/syntax/symbols/ParameterSymbol.h>

#include <new>
#include <vector>
#include <string>
#include <cmath>
#include <algorithm>
#include <Windows.h>

#include "PrimitivesLoading.h"

using namespace shard::framework;
using namespace shard::runtime;
using namespace shard::syntax;
using namespace shard::syntax::symbols;
using namespace shard::parsing::semantic;

// Integer methods
static ObjectInstance* ToString(const MethodSymbol* symbol, InboundVariablesContext* arguments)
{
	ObjectInstance* instance = arguments->TryFind(L"this");
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
