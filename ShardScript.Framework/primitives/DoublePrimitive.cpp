#include <new>
#include <vector>
#include <string>

#include <ShardScript.hpp>
#include <primitives/PrimitivesLoading.hpp>

using namespace shard;

// TODO: fix
/*
// Integer methods
static ObjectInstance* ToString(const VirtualMachine* host, const MethodSymbol* method, ArgumentsSpan& arguments)
{
	const ObjectInstance* instance = arguments[0];
	double value = instance->AsDouble();
	std::wstring str = std::to_wstring(value);
	return ObjectInstance::FromValue(str);
}
*/

void DoublePrimitive::Reflect(TypeSymbol* symbol)
{
	/*
	// ToString()
	MethodSymbol* toString = new MethodSymbol(L"ToString", ToString);
	toString->Accesibility = SymbolAccesibility::Public;
	toString->ReturnType = SymbolTable::Primitives::String;
	toString->IsStatic = false;
	symbol->Methods.push_back(toString);
	*/
}
