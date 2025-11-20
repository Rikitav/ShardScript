#include <shard/runtime/AbstractInterpreter.h>
#include <shard/runtime/InboundVariablesContext.h>
#include <shard/runtime/ObjectInstance.h>

#include <shard/framework/primitives/BooleanPrimitive.h>
#include <shard/parsing/semantic/SymbolTable.h>

#include <shard/syntax/SymbolAccesibility.h>
#include <shard/syntax/symbols/TypeSymbol.h>
#include <shard/syntax/symbols/MethodSymbol.h>

#include <string>

using namespace shard::framework;
using namespace shard::runtime;
using namespace shard::syntax;
using namespace shard::syntax::symbols;
using namespace shard::parsing::semantic;

// Boolean methods
static ObjectInstance* ToString(InboundVariablesContext* arguments)
{
	ObjectInstance* instance = arguments->TryFind(L"this");
	bool value = instance->ReadPrimitive<bool>();
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
