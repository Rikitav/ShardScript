#include <shard/syntax/symbols/ArrayTypeSymbol.h>
#include <shard/syntax/symbols/FieldSymbol.h>
#include <shard/syntax/symbols/MethodSymbol.h>
#include <shard/syntax/symbols/PropertySymbol.h>
#include <shard/syntax/symbols/TypeSymbol.h>

#include <shard/parsing/semantic/SymbolTable.h>

#include <vector>
#include <string>

using namespace shard::parsing::semantic;
using namespace shard::syntax::symbols;

MethodSymbol* ArrayTypeSymbol::FindMethod(std::wstring& name, std::vector<TypeSymbol*> parameterTypes)
{
	return SymbolTable::Primitives::Array->FindMethod(name, parameterTypes);
}

MethodSymbol* ArrayTypeSymbol::FindIndexator(std::vector<TypeSymbol*> parameterTypes)
{
	return SymbolTable::Primitives::Array->FindIndexator(parameterTypes);
}

FieldSymbol* ArrayTypeSymbol::FindField(std::wstring& name)
{
	return SymbolTable::Primitives::Array->FindField(name);
}

PropertySymbol* ArrayTypeSymbol::FindProperty(std::wstring& name)
{
	return SymbolTable::Primitives::Array->FindProperty(name);
}
