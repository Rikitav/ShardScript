#include <shard/syntax/symbols/TypeSymbol.h>
#include <shard/syntax/symbols/FieldSymbol.h>
#include <shard/syntax/symbols/MethodSymbol.h>
#include <shard/syntax/symbols/PropertySymbol.h>
#include <shard/syntax/symbols/IndexatorSymbol.h>
#include <shard/syntax/symbols/ParameterSymbol.h>
#include <shard/syntax/symbols/ArrayTypeSymbol.h>
#include <shard/syntax/symbols/DelegateTypeSymbol.h>

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
	switch (left->Kind)
	{
		default:
			return left->TypeCode == right->TypeCode;

		case SyntaxKind::DelegateType:
		{
			if (right->Kind != SyntaxKind::DelegateType)
				return false;

			const DelegateTypeSymbol* thisLambdaInfo = static_cast<const DelegateTypeSymbol*>(left);
			const DelegateTypeSymbol* otherLambdaInfo = static_cast<const DelegateTypeSymbol*>(right);
			
			if (thisLambdaInfo->ReturnType == nullptr || otherLambdaInfo->ReturnType == nullptr)
				return false;

			if (!TypeSymbol::Equals(thisLambdaInfo->ReturnType, otherLambdaInfo->ReturnType))
				return false;

			size_t size = thisLambdaInfo->Parameters.size();
			if (otherLambdaInfo->Parameters.size() != size)
				return false;

			for (size_t i = 0; i < size; i++)
			{
				ParameterSymbol* thisParam = thisLambdaInfo->Parameters.at(i);
				ParameterSymbol* otherParam = otherLambdaInfo->Parameters.at(i);

				if (!TypeSymbol::Equals(thisParam->Type, otherParam->Type))
					return true;
			}

			return true;
		}

		case SyntaxKind::ArrayType:
		{
			if (right->Kind != SyntaxKind::ArrayType)
				return false;

			const ArrayTypeSymbol* thisArrayInfo = static_cast<const ArrayTypeSymbol*>(left);
			const ArrayTypeSymbol* otherArrayInfo = static_cast<const ArrayTypeSymbol*>(right);

			if (thisArrayInfo->UnderlayingType == nullptr || otherArrayInfo->UnderlayingType == nullptr)
				return false;

			return Equals(thisArrayInfo->UnderlayingType, otherArrayInfo->UnderlayingType);
		}
	}
}

bool TypeSymbol::IsPrimitive()
{
	return this == SymbolTable::Primitives::Boolean
		|| this == SymbolTable::Primitives::Integer
		|| this == SymbolTable::Primitives::Char
		|| this == SymbolTable::Primitives::String;
}

MethodSymbol* TypeSymbol::FindConstructor(std::vector<TypeSymbol*> parameterTypes)
{
	for (MethodSymbol* symbol : Constructors)
	{
		if (symbol->Parameters.size() != parameterTypes.size())
			continue;

		if (std::equal(symbol->Parameters.begin(), symbol->Parameters.end(), parameterTypes.begin(), paramPredicate))
			return symbol;
	}

	return nullptr;
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

IndexatorSymbol* TypeSymbol::FindIndexator(std::vector<TypeSymbol*> parameterTypes)
{
	for (IndexatorSymbol* symbol : Indexators)
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
