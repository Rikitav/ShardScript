#include <shard/semantic/symbols/TypeSymbol.hpp>
#include <shard/semantic/symbols/FieldSymbol.hpp>
#include <shard/semantic/symbols/MethodSymbol.hpp>
#include <shard/semantic/symbols/PropertySymbol.hpp>
#include <shard/semantic/symbols/IndexatorSymbol.hpp>
#include <shard/semantic/symbols/ParameterSymbol.hpp>
#include <shard/semantic/symbols/ArrayTypeSymbol.hpp>
#include <shard/semantic/symbols/InterfaceSymbol.hpp>
#include <shard/semantic/symbols/DelegateTypeSymbol.hpp>
#include <shard/semantic/symbols/GenericTypeSymbol.hpp>
#include <shard/semantic/symbols/TypeParameterSymbol.hpp>
#include <shard/semantic/symbols/ConstructorSymbol.hpp>
#include <shard/semantic/symbols/OperatorSymbol.hpp>

#include <shard/semantic/SyntaxSymbol.hpp>
#include <shard/parsing/SyntaxKind.hpp>
#include <shard/parsing/SyntaxFacts.hpp>

#include <shard/semantic/SymbolTable.hpp>

#include <algorithm>
#include <vector>
#include <string>
#include <stdexcept>

using namespace shard;

std::size_t TypeSymbol::GetInlineSize() const
{
	return Inlining == TypeInlining::ByReference ? sizeof(void*) : MemoryBytesSize;
}

static bool paramPredicate(ParameterSymbol* left, TypeSymbol* right)
{
	if (left->Type == SymbolTable::Primitives::Any)
		return true;

	return TypeSymbol::IsAssignableFrom(left->Type, right);
}

bool TypeSymbol::Equals(const TypeSymbol* left, const TypeSymbol* right)
{
	if (left == nullptr || right == nullptr)
		return false;

	switch (left->Kind)
	{
		default:
			return left->DefinitionIndex == right->DefinitionIndex;

		case SyntaxKind::GenericType:
		{
			if (right->Kind != SyntaxKind::GenericType)
				return false;

			GenericTypeSymbol* thisGenericInfo = const_cast<GenericTypeSymbol*>(static_cast<const GenericTypeSymbol*>(left));
			GenericTypeSymbol* otherGenericInfo = const_cast<GenericTypeSymbol*>(static_cast<const GenericTypeSymbol*>(right));
			
			if (!TypeSymbol::Equals(thisGenericInfo->UnderlayingType, otherGenericInfo->UnderlayingType))
				return false;

			std::size_t size = thisGenericInfo->TypeParameters.size();
			if (otherGenericInfo->TypeParameters.size() != size)
				return false;

			for (std::size_t i = 0; i < size; i++)
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

			std::size_t size = thisLambdaInfo->Parameters.size();
			if (otherLambdaInfo->Parameters.size() != size)
				return false;

			for (std::size_t i = 0; i < size; i++)
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

static bool IsConstructedInterfaceImplemented(GenericTypeSymbol* targetInterface, TypeSymbol* sourceType)
{
    TypeSymbol* underlying = targetInterface->UnderlayingType;
    if (underlying->Kind != SyntaxKind::InterfaceDeclaration)
        return false;

    auto matchesInterface = [&](GenericTypeSymbol* candidateInterface) -> bool
    {
        if (candidateInterface->UnderlayingType != underlying)
            return false;

        for (TypeParameterSymbol* param : underlying->TypeParameters)
        {
            TypeSymbol* targetArg = targetInterface->SubstituteTypeParameters(param);
            TypeSymbol* sourceArg = candidateInterface->SubstituteTypeParameters(param);
            if (sourceArg != nullptr && sourceArg->Kind == SyntaxKind::TypeParameter && sourceType != nullptr && sourceType->Kind == SyntaxKind::GenericType)
                sourceArg = static_cast<GenericTypeSymbol*>(sourceType)->SubstituteTypeParameters(static_cast<TypeParameterSymbol*>(sourceArg));

            if (!TypeSymbol::Equals(targetArg, sourceArg))
                return false;
        }

        return true;
    };

    for (TypeSymbol* iface : sourceType->Interfaces)
    {
        if (iface->Kind != SyntaxKind::GenericType)
            continue;

        if (matchesInterface(static_cast<GenericTypeSymbol*>(iface)))
            return true;
    }

    if (sourceType->Kind == SyntaxKind::GenericType)
    {
        for (TypeSymbol* iface : static_cast<GenericTypeSymbol*>(sourceType)->UnderlayingType->Interfaces)
        {
            if (iface->Kind != SyntaxKind::GenericType)
                continue;

            if (matchesInterface(static_cast<GenericTypeSymbol*>(iface)))
                return true;
        }
    }

    if (sourceType->Kind == SyntaxKind::ArrayType)
    {
        for (TypeSymbol* iface : SymbolTable::Primitives::Array->Interfaces)
        {
            if (iface->Kind != SyntaxKind::GenericType)
                continue;

            if (matchesInterface(static_cast<GenericTypeSymbol*>(iface)))
                return true;
        }

        if (underlying == SymbolTable::StandardTypes::IEnumerable && underlying->TypeParameters.size() == 1)
        {
            TypeSymbol* targetArg = targetInterface->SubstituteTypeParameters(underlying->TypeParameters[0]);
            TypeSymbol* elementType = static_cast<ArrayTypeSymbol*>(sourceType)->UnderlayingType;
            if (TypeSymbol::Equals(targetArg, elementType))
                return true;
        }
    }

    return false;
}

bool TypeSymbol::IsAssignableFrom(const TypeSymbol* target, const TypeSymbol* source)
{
    if (target == nullptr || source == nullptr)
        return false;

    if (Equals(target, source))
        return true;

    if (source == SymbolTable::Primitives::Null)
        return target->Inlining == TypeInlining::ByReference || target == SymbolTable::Primitives::Any;

    if (target->Kind == SyntaxKind::InterfaceDeclaration)
    {
        for (TypeSymbol* iface : source->Interfaces)
        {
            if (Equals(target, iface))
                return true;
        }

        if (source->Kind == SyntaxKind::ArrayType)
        {
            for (TypeSymbol* iface : SymbolTable::Primitives::Array->Interfaces)
            {
                if (Equals(target, iface))
                    return true;
            }
        }
    }

    if (target->Kind == SyntaxKind::GenericType)
    {
        GenericTypeSymbol* targetGeneric = const_cast<GenericTypeSymbol*>(static_cast<const GenericTypeSymbol*>(target));
        if (IsConstructedInterfaceImplemented(targetGeneric, const_cast<TypeSymbol*>(source)))
            return true;
    }

    if (target->Kind == SyntaxKind::DelegateType && source->Kind == SyntaxKind::DelegateType)
    {
        const DelegateTypeSymbol* targetDelegate = static_cast<const DelegateTypeSymbol*>(target);
        const DelegateTypeSymbol* sourceDelegate = static_cast<const DelegateTypeSymbol*>(source);

        if (!Equals(targetDelegate->ReturnType, sourceDelegate->ReturnType))
            return false;

        if (targetDelegate->Parameters.size() != sourceDelegate->Parameters.size())
            return false;

        for (std::size_t i = 0; i < targetDelegate->Parameters.size(); ++i)
        {
            if (!Equals(targetDelegate->Parameters[i]->Type, sourceDelegate->Parameters[i]->Type))
                return false;
        }

        return true;
    }

    return false;
}

bool TypeSymbol::IsPrimitive()
{
	return this == SymbolTable::Primitives::Boolean
		|| this == SymbolTable::Primitives::Integer
		|| this == SymbolTable::Primitives::Double
		|| this == SymbolTable::Primitives::Char
		|| this == SymbolTable::Primitives::String
		|| this == SymbolTable::Primitives::Array
		|| this == SymbolTable::Primitives::NativeInteger;
}

void TypeSymbol::OnSymbolDeclared(SyntaxSymbol* symbol)
{
	switch (symbol->Kind)
	{
		case SyntaxKind::ConstructorDeclaration:
		{
			symbol->Parent = this;
			symbol->FullName = this->FullName + L"." + symbol->Name;
			
			ConstructorSymbol* ctor = static_cast<ConstructorSymbol*>(symbol);
			Constructors.push_back(ctor);
			break;
		}

		case SyntaxKind::MethodDeclaration:
		{
			symbol->Parent = this;
			symbol->FullName = this->FullName + L"." + symbol->Name;
			
			MethodSymbol* method = static_cast<MethodSymbol*>(symbol);
			Methods.push_back(method);
			break;
		}

		case SyntaxKind::OperatorDeclaration:
		{
			symbol->Parent = this;
			symbol->FullName = this->FullName + L"." + symbol->Name;
			
			OperatorSymbol* op = static_cast<OperatorSymbol*>(symbol);
			Operators.push_back(op);
			break;
		}

		case SyntaxKind::FieldDeclaration:
		{
			symbol->Parent = this;
			symbol->FullName = this->FullName + L"." + symbol->Name;
			
			FieldSymbol* field = static_cast<FieldSymbol*>(symbol);
			Fields.push_back(field);
			break;
		}

		case SyntaxKind::PropertyDeclaration:
		{
			symbol->Parent = this;
			symbol->FullName = this->FullName + L"." + symbol->Name;
			
			PropertySymbol* prop = static_cast<PropertySymbol*>(symbol);
			Properties.push_back(prop);
			break;
		}

		case SyntaxKind::IndexatorDeclaration:
		{
			symbol->Parent = this;
			symbol->FullName = this->FullName + L"." + symbol->Name;
			
			IndexatorSymbol* index = static_cast<IndexatorSymbol*>(symbol);
			Indexators.push_back(index);
			break;
		}

		case SyntaxKind::TypeParameter:
		{
			symbol->Parent = this;
			symbol->FullName = this->FullName + L"." + symbol->Name;

			TypeParameterSymbol* typeParam = static_cast<TypeParameterSymbol*>(symbol);
			TypeParameters.push_back(typeParam);
			break;
		}
	}
}

bool TypeSymbol::IsType() const
{
	return true;
}

bool TypeSymbol::IsMember() const
{
	return false;
}

ConstructorSymbol* TypeSymbol::FindConstructor(const std::vector<TypeSymbol*>& parameterTypes)
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

MethodSymbol* TypeSymbol::FindMethod(std::wstring& name, const std::vector<TypeSymbol*>& parameterTypes)
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

OperatorSymbol* TypeSymbol::FindOperator(TokenType opToken, const std::vector<TypeSymbol*>& parameterTypes)
{
	std::wstring opName = GetOperatorMethodName(opToken);
	if (opName.empty())
		return nullptr;

	for (OperatorSymbol* symbol : Operators)
	{
		if (symbol->Name != opName)
			continue;

		if (symbol->Parameters.size() != parameterTypes.size())
			continue;

		if (std::equal(symbol->Parameters.begin(), symbol->Parameters.end(), parameterTypes.begin(), paramPredicate))
			return symbol;
	}

	return nullptr;
}

IndexatorSymbol* TypeSymbol::FindIndexator(const std::vector<TypeSymbol*>& parameterTypes)
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

MethodSymbol* TypeSymbol::FindInterfaceImplementation(MethodSymbol* interfaceMethod)
{
	auto it = InterfaceMethodMap.find(interfaceMethod);
	return it == InterfaceMethodMap.end() ? nullptr : it->second;
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
				newArrayType->Length = arrayType->Length;
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

TypeSymbol* TypeSymbol::ReturnOf(ConstructorSymbol* constructor)
{
	if (constructor == nullptr || constructor->ReturnType == nullptr)
		return nullptr;

	TypeSymbol* resolvedType = SubstituteType(constructor->ReturnType);
	return resolvedType;
}
