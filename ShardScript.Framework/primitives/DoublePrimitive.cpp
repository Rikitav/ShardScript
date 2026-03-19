#include <new>
#include <vector>
#include <string>

#include <ShardScript.hpp>
#include <primitives/PrimitivesLoading.hpp>

using namespace shard;

// Integer methods
static ObjectInstance* ToString(const CallState& context)
{
	const ObjectInstance* instance = context.Args[0];
	double value = instance->AsDouble();
	std::wstring str = std::to_wstring(value);
	return context.Collector.FromValue(str);
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
