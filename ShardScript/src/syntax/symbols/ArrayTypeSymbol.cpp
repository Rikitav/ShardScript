#include <shard/syntax/symbols/ArrayTypeSymbol.h>
#include <shard/syntax/symbols/FieldSymbol.h>
#include <shard/syntax/symbols/MethodSymbol.h>
#include <shard/syntax/symbols/PropertySymbol.h>
#include <shard/syntax/symbols/ParameterSymbol.h>
#include <shard/parsing/semantic/SymbolTable.h>

#include <vector>
#include <string>

using namespace std;
using namespace shard::parsing::semantic;
using namespace shard::syntax::symbols;

MethodSymbol* ArrayTypeSymbol::FindMethod(wstring& name, vector<TypeSymbol*> parameterTypes)
{
	return SymbolTable::Primitives::Array->FindMethod(name, parameterTypes);
}

MethodSymbol* ArrayTypeSymbol::FindIndexator(vector<TypeSymbol*> parameterTypes)
{
	return SymbolTable::Primitives::Array->FindIndexator(parameterTypes);
}

FieldSymbol* ArrayTypeSymbol::FindField(wstring& name)
{
	return SymbolTable::Primitives::Array->FindField(name);
}

PropertySymbol* ArrayTypeSymbol::FindProperty(wstring& name)
{
	return SymbolTable::Primitives::Array->FindProperty(name);
}
