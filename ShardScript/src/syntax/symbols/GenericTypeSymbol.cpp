#include <shard/syntax/symbols/GenericTypeSymbol.h>
#include <shard/syntax/symbols/FieldSymbol.h>
#include <shard/syntax/symbols/MethodSymbol.h>
#include <shard/syntax/symbols/PropertySymbol.h>
#include <shard/syntax/symbols/TypeSymbol.h>

#include <shard/parsing/semantic/SymbolTable.h>

#include <vector>
#include <string>

using namespace shard::parsing::semantic;
using namespace shard::syntax::symbols;

MethodSymbol* GenericTypeSymbol::FindMethod(std::wstring& name, std::vector<TypeSymbol*> parameterTypes)
{
	return UnderlayingType->FindMethod(name, parameterTypes);
}

MethodSymbol* GenericTypeSymbol::FindIndexator(std::vector<TypeSymbol*> parameterTypes)
{
	return UnderlayingType->FindIndexator(parameterTypes);
}

FieldSymbol* GenericTypeSymbol::FindField(std::wstring& name)
{
	return UnderlayingType->FindField(name);
}

PropertySymbol* GenericTypeSymbol::FindProperty(std::wstring& name)
{
	return UnderlayingType->FindProperty(name);
}
