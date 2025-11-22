#include <shard/syntax/symbols/TypeSymbol.h>
#include <shard/syntax/symbols/FieldSymbol.h>
#include <shard/syntax/symbols/MethodSymbol.h>
#include <shard/syntax/symbols/PropertySymbol.h>
#include <shard/syntax/symbols/ParameterSymbol.h>
#include <shard/syntax/symbols/ArrayTypeSymbol.h>

#include <shard/syntax/SyntaxKind.h>

#include <shard/parsing/semantic/SymbolTable.h>

#include <algorithm>
#include <vector>
#include <string>

using namespace shard::parsing::semantic;
using namespace shard::syntax::symbols;

static bool paramPredicate(ParameterSymbol* left, TypeSymbol* right)
{
	if (left->Type == SymbolTable::Primitives::Any)
		return true;

	return TypeSymbol::Equals(left->Type, right);
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

MethodSymbol* TypeSymbol::FindMethod(std::wstring& name, std::vector<TypeSymbol*> parameterTypes)
{
	for (MethodSymbol* symbol : Methods)
	{
		if (symbol->Name != name)
			continue;

		if (symbol->Parameters.size() != parameterTypes.size())
			continue;

		if (std::equal(symbol->Parameters.begin(), symbol->Parameters.end(), parameterTypes.begin(), paramPredicate))
			return symbol;
	}

	return nullptr;
}

MethodSymbol* TypeSymbol::FindIndexator(std::vector<TypeSymbol*> parameterTypes)
{
	for (MethodSymbol* symbol : Indexators)
	{
		if (symbol->Parameters.size() != parameterTypes.size())
			continue;

		if (std::equal(symbol->Parameters.begin(), symbol->Parameters.end(), parameterTypes.begin(), paramPredicate))
			return symbol;
	}

	return nullptr;
}

FieldSymbol* TypeSymbol::FindField(std::wstring& name)
{
	for (FieldSymbol* symbol : Fields)
	{
		if (symbol->Name != name)
			continue;
		
		return symbol;
	}

	return nullptr;
}

PropertySymbol* TypeSymbol::FindProperty(std::wstring& name)
{
	for (PropertySymbol* symbol : Properties)
	{
		if (symbol->Name != name)
			continue;
		
		return symbol;
	}

	return nullptr;
}
