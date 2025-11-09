#include <shard/framework/primitives/ArrayPrimitive.h>
#include <shard/syntax/symbols/PropertySymbol.h>
#include <shard/syntax/SymbolAccesibility.h>
#include <shard/runtime/GarbageCollector.h>
#include <shard/parsing/semantic/SymbolTable.h>

using namespace shard::framework;
using namespace shard::syntax;
using namespace shard::runtime;
using namespace shard::syntax::symbols;
using namespace shard::parsing::semantic;

ObjectInstance* get_Length(InboundVariablesContext* arguments)
{
	ObjectInstance* instance = arguments->TryFind(L"this");
	void* value = malloc(sizeof(int));
	instance->ReadMemory(0, sizeof(int), value);

	ObjectInstance* length = GarbageCollector::AllocateInstance(SymbolTable::Primitives::Integer);
	length->WriteMemory(0, sizeof(int), value);
	return length;
}

ObjectInstance* indexator(InboundVariablesContext* arguments)
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
	lengthProperty->GetMethod = new MethodSymbol(L"get_Length", get_Length);

	symbol->Properties.push_back(lengthProperty);

	MethodSymbol* indexatorMethod = new MethodSymbol(L"", indexator);
	indexatorMethod->Accesibility = SymbolAccesibility::Public;
	
	ParameterSymbol* indexatorParam = new ParameterSymbol(L"index");
	indexatorParam->Type = SymbolTable::Primitives::Integer;
	indexatorMethod->Parameters.push_back(indexatorParam);

	symbol->Indexators.push_back(indexatorMethod);
}
