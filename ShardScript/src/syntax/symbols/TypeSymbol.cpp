#include <shard/syntax/symbols/TypeSymbol.hpp>
#include <shard/syntax/symbols/FieldSymbol.hpp>
#include <shard/syntax/symbols/MethodSymbol.hpp>
#include <shard/syntax/symbols/PropertySymbol.hpp>
#include <shard/syntax/symbols/IndexatorSymbol.hpp>
#include <shard/syntax/symbols/ParameterSymbol.hpp>
#include <shard/syntax/symbols/ArrayTypeSymbol.hpp>
#include <shard/syntax/symbols/DelegateTypeSymbol.hpp>
#include <shard/syntax/symbols/GenericTypeSymbol.hpp>
#include <shard/syntax/symbols/ConstructorSymbol.hpp>

#include <shard/syntax/SyntaxSymbol.hpp>
#include <shard/syntax/SyntaxKind.hpp>

#include <shard/parsing/semantic/SymbolTable.hpp>

#include <algorithm>
#include <vector>
#include <string>
#include <stdexcept>

using namespace shard;

TypeSymbol::TypeSymbol(const std::wstring& name, const SyntaxKind kind) : SyntaxSymbol(name, kind)
{

}

TypeSymbol::~TypeSymbol()
{
#pragma warning (push)
#pragma warning (disable: 4150)
	for (MethodSymbol* methodSymbol : Methods)
		delete methodSymbol;

	for (FieldSymbol* fieldSymbol : Fields)
		delete fieldSymbol;

	for (PropertySymbol* propertySymbol : Properties)
		delete propertySymbol;

	for (IndexatorSymbol* indexatorSymbol : Indexators)
		delete indexatorSymbol;

	for (ConstructorSymbol* ctorSymbol : Constructors)
		delete ctorSymbol;

	for (TypeParameterSymbol* typeParamSymbol : TypeParameters)
		delete typeParamSymbol;
#pragma warning (pop)

	if (BaseType != nullptr)
		BaseType = nullptr;
}

size_t TypeSymbol::GetInlineSize() const
{
	return IsReferenceType ? sizeof(void*) : MemoryBytesSize;
}

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

		case SyntaxKind::GenericType:
		{
			if (right->Kind != SyntaxKind::GenericType)
				return false;

			GenericTypeSymbol* thisGenericInfo = const_cast<GenericTypeSymbol*>(static_cast<const GenericTypeSymbol*>(left));
			GenericTypeSymbol* otherGenericInfo = const_cast<GenericTypeSymbol*>(static_cast<const GenericTypeSymbol*>(right));
			
			if (!TypeSymbol::Equals(thisGenericInfo->UnderlayingType, otherGenericInfo->UnderlayingType))
				return false;

			size_t size = thisGenericInfo->TypeParameters.size();
			if (otherGenericInfo->TypeParameters.size() != size)
				return false;

			for (size_t i = 0; i < size; i++)
			{
				TypeSymbol* thisParam = thisGenericInfo->SubstituteTypeParameters(thisGenericInfo->TypeParameters.at(i));
				TypeSymbol* otherParam = otherGenericInfo->SubstituteTypeParameters(otherGenericInfo->TypeParameters.at(i));

				if (!TypeSymbol::Equals(thisParam, otherParam))
					return false;
			}

			return true;
		}

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
					return false;
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
		|| this == SymbolTable::Primitives::Double
		|| this == SymbolTable::Primitives::Char
		|| this == SymbolTable::Primitives::String;
}

void TypeSymbol::OnSymbolDeclared(SyntaxSymbol* symbol)
{
	switch (symbol->Kind)
	{
		case SyntaxKind::ConstructorDeclaration:
		{
			ConstructorSymbol* ctor = static_cast<ConstructorSymbol*>(symbol);
			Constructors.push_back(ctor);
			break;
		}

		case SyntaxKind::MethodDeclaration:
		{
			MethodSymbol* method = static_cast<MethodSymbol*>(symbol);
			Methods.push_back(method);
			break;
		}

		case SyntaxKind::FieldDeclaration:
		{
			FieldSymbol* field = static_cast<FieldSymbol*>(symbol);
			Fields.push_back(field);
			break;
		}

		case SyntaxKind::PropertyDeclaration:
		{
			PropertySymbol* prop = static_cast<PropertySymbol*>(symbol);
			Properties.push_back(prop);
			break;
		}

		case SyntaxKind::IndexatorDeclaration:
		{
			IndexatorSymbol* index = static_cast<IndexatorSymbol*>(symbol);
			Indexators.push_back(index);
			break;
		}

		default:
		{
			/*
			if (symbol->IsType())
			{
				TypeSymbol* type = static_cast<TypeSymbol*>(symbol);
				Members.push_back(type);
			}
			*/

			break;
		}
	}
}

ConstructorSymbol* TypeSymbol::FindConstructor(std::vector<TypeSymbol*> parameterTypes)
{
	for (ConstructorSymbol* symbol : Constructors)
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

TypeSymbol* TypeSymbol::SubstituteType(TypeSymbol* type)
{
	if (type == nullptr)
		return nullptr;

	switch (type->Kind)
	{
		default:
			throw std::runtime_error("unknown syntax kind to substitute");

		case SyntaxKind::StructDeclaration:
		case SyntaxKind::ClassDeclaration:
			return type;

		case SyntaxKind::ArrayType:
		{
			ArrayTypeSymbol* arrayType = static_cast<ArrayTypeSymbol*>(type);
			TypeSymbol* underlayingType = arrayType->UnderlayingType;

			if (underlayingType == nullptr)
				throw std::runtime_error("Cannot resolve underlaying type of array");

			if (underlayingType->Kind == SyntaxKind::TypeParameter)
			{
				underlayingType = SubstituteType(arrayType->UnderlayingType);
				if (underlayingType == nullptr)
					throw std::runtime_error("Cannot resolve underlaying type of array");

				ArrayTypeSymbol* newArrayType = new ArrayTypeSymbol(underlayingType);
				newArrayType->Size = arrayType->Size;
				newArrayType->Rank = arrayType->Rank;
				return newArrayType;
			}

			return arrayType;
		}

		case SyntaxKind::TypeParameter:
		{
			return nullptr;

			//TODO: FIX
			/*
			CallStackFrame* frame = AbstractInterpreter::CurrentFrame();
			if (frame == nullptr)
				throw std::runtime_error("Cannot get current call stack frame");

			if (frame->WithinType == nullptr)
				throw std::runtime_error("Cannot resolve within null type");

			if (frame->WithinType->Kind != SyntaxKind::GenericType)
				throw std::runtime_error("Cannot resolve generic type parameter within non generic type symbol");

			GenericTypeSymbol* genericType = const_cast<GenericTypeSymbol*>(static_cast<const GenericTypeSymbol*>(frame->WithinType));
			TypeSymbol* resolvedType = genericType->SubstituteTypeParameters(type);
			return resolvedType;
			*/
		}
	}
}

TypeSymbol* TypeSymbol::ReturnOf(FieldSymbol* field)
{
	if (field == nullptr || field->ReturnType == nullptr)
		return nullptr;

	TypeSymbol* resolvedType = SubstituteType(field->ReturnType);
	return resolvedType;
}

TypeSymbol* TypeSymbol::ReturnOf(MethodSymbol* method)
{
	if (method == nullptr || method->ReturnType == nullptr)
		return nullptr;

	TypeSymbol* resolvedType = SubstituteType(method->ReturnType);
	return resolvedType;
}

TypeSymbol* TypeSymbol::ReturnOf(PropertySymbol* property)
{
	if (property == nullptr || property->ReturnType == nullptr)
		return nullptr;

	TypeSymbol* resolvedType = SubstituteType(property->ReturnType);
	return resolvedType;
}

TypeSymbol* TypeSymbol::ReturnOf(IndexatorSymbol* indexator)
{
	if (indexator == nullptr || indexator->ReturnType == nullptr)
		return nullptr;

	TypeSymbol* resolvedType = SubstituteType(indexator->ReturnType);
	return resolvedType;
}

TypeSymbol* TypeSymbol::ReturnOf(ConstructorSymbol* constructor)
{
	if (constructor == nullptr || constructor->ReturnType == nullptr)
		return nullptr;

	TypeSymbol* resolvedType = SubstituteType(constructor->ReturnType);
	return resolvedType;
}
