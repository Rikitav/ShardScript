#include <string>

#include <ShardScript.hpp>
#include <primitives/PrimitivesLoading.hpp>

using namespace shard;

// Boolean methods
static ObjectInstance* ToString(const CallState& context)
{
	ObjectInstance* instance = context.Args[0]; // this
	bool value = instance->AsBoolean();
	std::wstring str = value ? L"true" : L"false";
	return context.Collector.FromValue(str);
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
