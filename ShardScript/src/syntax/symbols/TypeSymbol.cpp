#include <shard/syntax/symbols/TypeSymbol.h>
#include <shard/syntax/symbols/FieldSymbol.h>
#include <shard/syntax/symbols/MethodSymbol.h>
#include <shard/syntax/symbols/ParameterSymbol.h>

#include <algorithm>
#include <vector>
#include <string>

using namespace std;
using namespace shard::syntax::symbols;

static bool paramPredicate(TypeSymbol* left, ParameterSymbol* right)
{
	return left == right->Type;
}

MethodSymbol* TypeSymbol::FindMethod(wstring& name, vector<TypeSymbol*> parameterTypes)
{
	for (MethodSymbol* symbol : Methods)
	{
		if (symbol->Name != name)
			continue;

		if (symbol->Parameters.size() != parameterTypes.size())
			continue;

		if (std::equal(parameterTypes.begin(), parameterTypes.end(), symbol->Parameters.begin(), paramPredicate))
			return symbol;
	}

	return nullptr;
}

FieldSymbol* TypeSymbol::FindField(wstring& name)
{
	for (FieldSymbol* symbol : Fields)
	{
		if (symbol->Name != name)
			continue;
		
		return symbol;
	}

	return nullptr;
}
