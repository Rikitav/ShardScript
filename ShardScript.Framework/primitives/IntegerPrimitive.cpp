#include <shard/runtime/ArgumentsSpan.h>
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
#include <cstdint>
#include <cstdlib>
#include <algorithm>

#include "PrimitivesLoading.h"

using namespace shard;

// Integer methods
static ObjectInstance* ToString(const VirtualMachine* host, const MethodSymbol* method, ArgumentsSpan& arguments)
{
	int64_t value = arguments[0]->AsInteger(); // this
	std::wstring str = std::to_wstring(value);
	return ObjectInstance::FromValue(str);
}

static ObjectInstance* Abs(const VirtualMachine* host, const MethodSymbol* method, ArgumentsSpan& arguments)
{
	int64_t value = arguments[0]->AsInteger(); // this
	int64_t result = abs(value);
	return ObjectInstance::FromValue(result);
}

static ObjectInstance* Min(const VirtualMachine* host, const MethodSymbol* method, ArgumentsSpan& arguments)
{
	int64_t value = arguments[0]->AsInteger(); // this
	int64_t other = arguments[1]->AsInteger(); // other

	int64_t result = std::min(value, other);
	return ObjectInstance::FromValue(result);
}

static ObjectInstance* Max(const VirtualMachine* host, const MethodSymbol* method, ArgumentsSpan& arguments)
{
	int64_t value = arguments[0]->AsInteger(); // this
	int64_t other = arguments[1]->AsInteger(); // other

	int64_t result = std::max(value, other);
	return ObjectInstance::FromValue(result);
}

static ObjectInstance* Pow(const VirtualMachine* host, const MethodSymbol* method, ArgumentsSpan& arguments)
{
	int64_t value = arguments[0]->AsInteger(); // this
	int64_t power = arguments[1]->AsInteger(); // power

	int64_t result = static_cast<int64_t>(std::pow(value, power));
	return ObjectInstance::FromValue(result);
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
