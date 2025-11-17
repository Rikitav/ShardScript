#include <shard/syntax/symbols/TypeSymbol.h>
#include <shard/syntax/symbols/FieldSymbol.h>
#include <shard/syntax/symbols/MethodSymbol.h>
#include <shard/syntax/symbols/PropertySymbol.h>
#include <shard/syntax/symbols/ParameterSymbol.h>
#include <shard/syntax/symbols/ArrayTypeSymbol.h>
#include <shard/syntax/SyntaxKind.h>

#include <algorithm>
#include <vector>
#include <string>

using namespace std;
using namespace shard::syntax::symbols;

static bool paramPredicate(TypeSymbol* left, ParameterSymbol* right)
{
	return left == right->Type;
}

bool TypeSymbol::Equals(const TypeSymbol* left, const TypeSymbol* right)
{
	if (left->Kind == SyntaxKind::CollectionExpression)
	{
		if (right->Kind != SyntaxKind::CollectionExpression)
			return false;

		const ArrayTypeSymbol* thisArrayInfo = static_cast<const ArrayTypeSymbol*>(left);
		const ArrayTypeSymbol* otherArrayInfo = static_cast<const ArrayTypeSymbol*>(right);
		return thisArrayInfo->UnderlayingType->TypeCode == otherArrayInfo->UnderlayingType->TypeCode;
	}

	return left->TypeCode == right->TypeCode;
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

MethodSymbol* TypeSymbol::FindIndexator(vector<TypeSymbol*> parameterTypes)
{
	for (MethodSymbol* symbol : Indexators)
	{
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

PropertySymbol* TypeSymbol::FindProperty(wstring& name)
{
	for (PropertySymbol* symbol : Properties)
	{
		if (symbol->Name != name)
			continue;
		
		return symbol;
	}

	return nullptr;
}
