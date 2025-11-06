#include <shard/runtime/framework/primitives/ArrayPrimitive.h>
#include <shard/syntax/symbols/PropertySymbol.h>
#include <shard/syntax/SymbolAccesibility.h>
#include <shard/parsing/semantic/SymbolTable.h>

using namespace shard::syntax;
using namespace shard::syntax::symbols;
using namespace shard::parsing::semantic;

void ArrayPrimitive::Reflect(TypeSymbol* symbol)
{
	//PropertySymbol* lengthProperty = new PropertySymbol(L"Length");
	//lengthProperty->Accesibility = SymbolAccesibility::Public;
	//lengthProperty->ReturnType = SymbolTable::Primitives::Integer;
}
