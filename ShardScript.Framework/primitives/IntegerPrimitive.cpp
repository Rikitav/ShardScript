#include <new>
#include <vector>
#include <string>
#include <cmath>
#include <cstdint>
#include <cstdlib>

#include <Windows.h>
#include <ShardScript.hpp>
#include <primitives/PrimitivesLoading.hpp>

using namespace shard;

// Integer methods
static ObjectInstance* ToString(const CallState& context)
{
	int64_t value = context.Args[0]->AsInteger(); // this
	std::wstring str = std::to_wstring(value);
	return context.Collector.FromValue(str);
}

static ObjectInstance* Abs(const CallState& context)
{
	int64_t value = context.Args[0]->AsInteger(); // this
	int64_t result = abs(value);
	return context.Collector.FromValue(result);
}

static ObjectInstance* Min(const CallState& context)
{
	int64_t value = context.Args[0]->AsInteger(); // this
	int64_t other = context.Args[1]->AsInteger(); // other

	int64_t result = min(value, other);
	return context.Collector.FromValue(result);
}

static ObjectInstance* Max(const CallState& context)
{
	int64_t value = context.Args[0]->AsInteger(); // this
	int64_t other = context.Args[1]->AsInteger(); // other

	int64_t result = max(value, other);
	return context.Collector.FromValue(result);
}

static ObjectInstance* Pow(const CallState& context)
{
	int64_t value = context.Args[0]->AsInteger(); // this
	int64_t power = context.Args[1]->AsInteger(); // power

	int64_t result = static_cast<int64_t>(std::pow(value, power));
	return context.Collector.FromValue(result);
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
