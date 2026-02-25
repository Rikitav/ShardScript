#include <string>

#include <ShardScript.hpp>
#include <primitives/PrimitivesLoading.hpp>

using namespace shard;

// TODO: fix
/*
// Boolean methods
static ObjectInstance* ToString(const VirtualMachine* host, const MethodSymbol* method, ArgumentsSpan& arguments)
{
	ObjectInstance* instance = arguments[0]; // this
	bool value = instance->AsBoolean();
	std::wstring str = value ? L"true" : L"false";
	return ObjectInstance::FromValue(str);
}
*/

void BooleanPrimitive::Reflect(TypeSymbol* symbol)
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
