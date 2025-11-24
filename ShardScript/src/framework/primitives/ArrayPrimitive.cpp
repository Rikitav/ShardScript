#include <shard/framework/primitives/ArrayPrimitive.h>

#include <shard/syntax/SymbolAccesibility.h>
#include <shard/parsing/semantic/SymbolTable.h>

#include <shard/runtime/GarbageCollector.h>
#include <shard/runtime/InboundVariablesContext.h>
#include <shard/runtime/ObjectInstance.h>

#include <shard/syntax/symbols/MethodSymbol.h>
#include <shard/syntax/symbols/ParameterSymbol.h>
#include <shard/syntax/symbols/TypeSymbol.h>
#include <shard/syntax/symbols/PropertySymbol.h>

#include <malloc.h>

using namespace shard::framework;
using namespace shard::syntax;
using namespace shard::runtime;
using namespace shard::syntax::symbols;
using namespace shard::parsing::semantic;

static ObjectInstance* get_Length(MethodSymbol* symbol, InboundVariablesContext* arguments)
{
	ObjectInstance* instance = arguments->TryFind(L"this");
	void* value = malloc(sizeof(int));
	instance->ReadMemory(0, sizeof(int), value);

	ObjectInstance* length = GarbageCollector::AllocateInstance(SymbolTable::Primitives::Integer);
	length->WriteMemory(0, sizeof(int), value);
	return length;
}

static ObjectInstance* indexator(MethodSymbol* symbol, InboundVariablesContext* arguments)
{
	ObjectInstance* instance = arguments->TryFind(L"this");
	ObjectInstance* index = arguments->TryFind(L"index");
	int value = index->ReadPrimitive<int>();
	return instance->GetElement(value);
}

void ArrayPrimitive::Reflect(TypeSymbol* symbol)
{
	PropertySymbol* lengthProperty = new PropertySymbol(L"Length");
	lengthProperty->Accesibility = SymbolAccesibility::Public;
	lengthProperty->ReturnType = SymbolTable::Primitives::Integer;

	lengthProperty->Getter = new AccessorSymbol(L"get_Length");
	lengthProperty->Getter->Accesibility = SymbolAccesibility::Public;
	lengthProperty->Getter->Method = new MethodSymbol(L"get_Length", get_Length);

	symbol->Properties.push_back(lengthProperty);

	MethodSymbol* indexatorMethod = new MethodSymbol(L"", indexator);
	indexatorMethod->Accesibility = SymbolAccesibility::Public;
	
	ParameterSymbol* indexatorParam = new ParameterSymbol(L"index");
	indexatorParam->Type = SymbolTable::Primitives::Integer;
	indexatorMethod->Parameters.push_back(indexatorParam);

	symbol->Indexators.push_back(indexatorMethod);
}
