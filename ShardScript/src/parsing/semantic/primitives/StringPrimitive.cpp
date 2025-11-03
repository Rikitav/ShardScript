#include <shard/runtime/interpreter/PrimitiveMathModule.h>
#include <shard/runtime/InboundVariablesContext.h>
#include <shard/runtime/ObjectInstance.h>

#include <shard/parsing/semantic/primitives/StringPrimitive.h>
#include <shard/parsing/semantic/SymbolTable.h>

#include <shard/syntax/SymbolAccesibility.h>
#include <shard/syntax/symbols/TypeSymbol.h>
#include <shard/syntax/symbols/MethodSymbol.h>

#include <string>

using namespace std;
using namespace shard::runtime;
using namespace shard::syntax;
using namespace shard::syntax::symbols;
using namespace shard::parsing::semantic;

static ObjectInstance* IsEmpty(InboundVariablesContext* arguments)
{
	ObjectInstance* instance = arguments->TryFind(L"this");
	wstring value = instance->ReadPrimitive<wstring>();
	return PrimitiveMathModule::CreateInstanceFromValue(value.size() == 0);
}

void StringPrimitive::Reflect(TypeSymbol* symbol)
{
	MethodSymbol* methodSymbol_Boolean_IsEmpty = new MethodSymbol(L"IsEmpty", IsEmpty);
	methodSymbol_Boolean_IsEmpty->Accesibility = SymbolAccesibility::Public;
	methodSymbol_Boolean_IsEmpty->ReturnType = SymbolTable::Primitives::Boolean;
	symbol->Methods.push_back(methodSymbol_Boolean_IsEmpty);
}
