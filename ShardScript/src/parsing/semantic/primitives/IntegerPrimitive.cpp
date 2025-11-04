#include <shard/runtime/interpreter/PrimitiveMathModule.h>
#include <shard/runtime/InboundVariablesContext.h>
#include <shard/runtime/ObjectInstance.h>

#include <shard/parsing/semantic/primitives/IntegerPrimitive.h>
#include <shard/parsing/semantic/SymbolTable.h>

#include <shard/syntax/SymbolAccesibility.h>
#include <shard/syntax/symbols/TypeSymbol.h>
#include <shard/syntax/symbols/MethodSymbol.h>
#include <shard/syntax/symbols/ParameterSymbol.h>

#include <string>
#include <cmath>

using namespace std;
using namespace shard::runtime;
using namespace shard::syntax;
using namespace shard::syntax::symbols;
using namespace shard::parsing::semantic;

// Integer methods
static ObjectInstance* ToString(InboundVariablesContext* arguments)
{
	ObjectInstance* instance = arguments->TryFind(L"this");
	int value = instance->ReadPrimitive<int>();
	wstring str = to_wstring(value);
	return PrimitiveMathModule::CreateInstanceFromValue(str);
}

static ObjectInstance* Abs(InboundVariablesContext* arguments)
{
	ObjectInstance* instance = arguments->TryFind(L"this");
	int value = instance->ReadPrimitive<int>();
	int result = abs(value);
	return PrimitiveMathModule::CreateInstanceFromValue(result);
}

static ObjectInstance* Min(InboundVariablesContext* arguments)
{
	ObjectInstance* instance = arguments->TryFind(L"this");
	int value = instance->ReadPrimitive<int>();
	
	ObjectInstance* otherArg = arguments->TryFind(L"other");
	if (otherArg == nullptr)
		return PrimitiveMathModule::CreateInstanceFromValue(value);
	
	int other = otherArg->ReadPrimitive<int>();
	int result = min(value, other);
	return PrimitiveMathModule::CreateInstanceFromValue(result);
}

static ObjectInstance* Max(InboundVariablesContext* arguments)
{
	ObjectInstance* instance = arguments->TryFind(L"this");
	int value = instance->ReadPrimitive<int>();
	
	ObjectInstance* otherArg = arguments->TryFind(L"other");
	if (otherArg == nullptr)
		return PrimitiveMathModule::CreateInstanceFromValue(value);
	
	int other = otherArg->ReadPrimitive<int>();
	int result = max(value, other);
	return PrimitiveMathModule::CreateInstanceFromValue(result);
}

static ObjectInstance* Pow(InboundVariablesContext* arguments)
{
	ObjectInstance* instance = arguments->TryFind(L"this");
	int value = instance->ReadPrimitive<int>();
	
	ObjectInstance* powerArg = arguments->TryFind(L"power");
	if (powerArg == nullptr)
		return PrimitiveMathModule::CreateInstanceFromValue(1);
	
	int power = powerArg->ReadPrimitive<int>();
	int result = static_cast<int>(pow(value, power));
	return PrimitiveMathModule::CreateInstanceFromValue(result);
}

void IntegerPrimitive::Reflect(TypeSymbol* symbol)
{
	// ToString()
	MethodSymbol* toString = new MethodSymbol(L"ToString", ToString);
	toString->Accesibility = SymbolAccesibility::Public;
	toString->ReturnType = SymbolTable::Primitives::String;
	toString->IsStatic = false;
	symbol->Methods.push_back(toString);
	
	// Abs()
	MethodSymbol* abs = new MethodSymbol(L"Abs", Abs);
	abs->Accesibility = SymbolAccesibility::Public;
	abs->ReturnType = SymbolTable::Primitives::Integer;
	abs->IsStatic = false;
	symbol->Methods.push_back(abs);
	
	// Min(other)
	MethodSymbol* min = new MethodSymbol(L"Min", Min);
	min->Accesibility = SymbolAccesibility::Public;
	min->ReturnType = SymbolTable::Primitives::Integer;
	min->IsStatic = false;
	ParameterSymbol* minParam = new ParameterSymbol(L"other");
	minParam->Type = SymbolTable::Primitives::Integer;
	min->Parameters.push_back(minParam);
	symbol->Methods.push_back(min);
	
	// Max(other)
	MethodSymbol* max = new MethodSymbol(L"Max", Max);
	max->Accesibility = SymbolAccesibility::Public;
	max->ReturnType = SymbolTable::Primitives::Integer;
	max->IsStatic = false;
	ParameterSymbol* maxParam = new ParameterSymbol(L"other");
	maxParam->Type = SymbolTable::Primitives::Integer;
	max->Parameters.push_back(maxParam);
	symbol->Methods.push_back(max);
	
	// Pow(power)
	MethodSymbol* pow = new MethodSymbol(L"Pow", Pow);
	pow->Accesibility = SymbolAccesibility::Public;
	pow->ReturnType = SymbolTable::Primitives::Integer;
	pow->IsStatic = false;
	ParameterSymbol* powParam = new ParameterSymbol(L"power");
	powParam->Type = SymbolTable::Primitives::Integer;
	pow->Parameters.push_back(powParam);
	symbol->Methods.push_back(pow);
}
