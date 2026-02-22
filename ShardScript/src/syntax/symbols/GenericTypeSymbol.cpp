#include <shard/syntax/symbols/GenericTypeSymbol.hpp>
#include <shard/syntax/symbols/FieldSymbol.hpp>
#include <shard/syntax/symbols/MethodSymbol.hpp>
#include <shard/syntax/symbols/PropertySymbol.hpp>
#include <shard/syntax/symbols/TypeSymbol.hpp>
#include <shard/syntax/symbols/IndexatorSymbol.hpp>
#include <shard/syntax/symbols/ParameterSymbol.hpp>
#include <shard/syntax/symbols/TypeParameterSymbol.hpp>

#include <shard/syntax/SyntaxKind.hpp>

#include <shard/parsing/semantic/SymbolTable.hpp>

#include <vector>
#include <string>
#include <algorithm>

using namespace shard;

void GenericTypeSymbol::AddTypeParameter(TypeParameterSymbol* typeParam, TypeSymbol* constraintType)
{
	_typeParametersMap[typeParam] = constraintType;
}

TypeSymbol* GenericTypeSymbol::SubstituteTypeParameters(TypeParameterSymbol* typeParam)
{
	auto find = _typeParametersMap.find(typeParam);
	return find == _typeParametersMap.end() ? nullptr : find->second;
}

MethodSymbol* GenericTypeSymbol::FindMethod(std::wstring& name, std::vector<TypeSymbol*> parameterTypes)
{
	static const auto predicate = [this](ParameterSymbol* left, TypeSymbol* right)
	{
		TypeSymbol* leftType = left->Type;
		if (leftType == SymbolTable::Primitives::Any)
			return true;

		if (leftType->Kind == SyntaxKind::TypeParameter)
			leftType = this->SubstituteTypeParameters(static_cast<TypeParameterSymbol*>(leftType));

		return TypeSymbol::Equals(leftType, right);
	};

	for (MethodSymbol* symbol : UnderlayingType->Methods)
	{
		if (symbol->Name != name)
			continue;

		if (symbol->Parameters.size() != parameterTypes.size())
			continue;

		if (std::equal(symbol->Parameters.begin(), symbol->Parameters.end(), parameterTypes.begin(), predicate))
			return symbol;
	}

	return nullptr;
}

IndexatorSymbol* GenericTypeSymbol::FindIndexator(std::vector<TypeSymbol*> parameterTypes)
{
	static const auto predicate = [this](ParameterSymbol* left, TypeSymbol* right)
	{
		TypeSymbol* leftType = left->Type;
		if (leftType == SymbolTable::Primitives::Any)
			return true;

		if (leftType->Kind == SyntaxKind::TypeParameter)
			leftType = this->SubstituteTypeParameters(static_cast<TypeParameterSymbol*>(leftType));

		return TypeSymbol::Equals(leftType, right);
	};

	for (IndexatorSymbol* symbol : UnderlayingType->Indexators)
	{
		if (symbol->Parameters.size() != parameterTypes.size())
			continue;

		if (std::equal(symbol->Parameters.begin(), symbol->Parameters.end(), parameterTypes.begin(), predicate))
			return symbol;
	}

	return nullptr;
}

FieldSymbol* GenericTypeSymbol::FindField(std::wstring& name)
{
	return UnderlayingType->FindField(name);
}

PropertySymbol* GenericTypeSymbol::FindProperty(std::wstring& name)
{
	return UnderlayingType->FindProperty(name);
}
