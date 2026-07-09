#include <shard/parsing/SyntaxKind.hpp>
#include <shard/parsing/SyntaxNode.hpp>
#include <shard/lexical/TokenType.hpp>

#include <shard/parsing/nodes/ExpressionSyntax.hpp>
#include <shard/parsing/nodes/Expressions/LiteralExpressionSyntax.hpp>

#include <shard/parsing/SyntaxTree.hpp>
#include <shard/semantic/SemanticModel.hpp>
#include <shard/semantic/NamespaceTree.hpp>
#include <shard/semantic/SymbolInfo.hpp>
#include <shard/semantic/SymbolTable.hpp>
#include <shard/semantic/TypeInfo.hpp>

#include <shard/semantic/symbols/ArrayTypeSymbol.hpp>
#include <shard/semantic/symbols/ConstructorSymbol.hpp>
#include <shard/semantic/symbols/DelegateTypeSymbol.hpp>
#include <shard/semantic/symbols/FieldSymbol.hpp>
#include <shard/semantic/symbols/GenericTypeSymbol.hpp>
#include <shard/semantic/symbols/IndexatorSymbol.hpp>
#include <shard/semantic/symbols/InterfaceSymbol.hpp>
#include <shard/semantic/symbols/MethodSymbol.hpp>
#include <shard/semantic/symbols/OperatorSymbol.hpp>
#include <shard/semantic/symbols/ParameterSymbol.hpp>
#include <shard/semantic/symbols/PropertySymbol.hpp>
#include <shard/semantic/symbols/TypeParameterSymbol.hpp>
#include <shard/semantic/symbols/TypeSymbol.hpp>

#include <stdexcept>
#include <memory>
#include <algorithm>

using namespace shard;

SemanticModel::SemanticModel(shard::SyntaxTree& tree) : Tree(tree)
{
	Table = std::make_unique<SymbolTable>();
	Namespaces = std::make_unique<NamespaceTree>();
	TypeShapes = std::make_unique<TypeShapeCache>();
	//Namespaces->Root = SymbolTable::Global::Namespace->Node;
}

SemanticModel::~SemanticModel()
{
}

SymbolInfo SemanticModel::GetSymbolInfo(SyntaxNode* node)
{
	auto symbol = Table->LookupSymbol(node);
	return SymbolInfo(symbol.value_or(nullptr));
}

TypeInfo SemanticModel::GetTypeInfo(ExpressionSyntax* expression)
{
	switch (expression->Kind)
	{
		default:
			throw std::runtime_error("unsupported expression type");

		case SyntaxKind::LiteralExpression:
		{
			LiteralExpressionSyntax* literal = static_cast<LiteralExpressionSyntax*>(expression);
			switch (literal->LiteralToken.Type)
			{
				case TokenType::BooleanLiteral:
					return SymbolTable::Primitives::Boolean;

				case TokenType::NumberLiteral:
					return SymbolTable::Primitives::Integer;

				case TokenType::DoubleLiteral:
					return SymbolTable::Primitives::Double;

				case TokenType::CharLiteral:
					return SymbolTable::Primitives::Char;

				case TokenType::StringLiteral:
					return SymbolTable::Primitives::String;

				default:
					throw std::runtime_error("unsupported literal type");
			}
		}
	}
}

// =========================================================================
//  Type system utilities
// =========================================================================

static bool IsConstructedInterfaceImplemented(GenericTypeSymbol* targetInterface, TypeSymbol* sourceType);

bool SemanticModel::AreTypesEqual(const TypeSymbol* left, const TypeSymbol* right)
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

			const GenericTypeSymbol* thisGenericInfo = static_cast<const GenericTypeSymbol*>(left);
			const GenericTypeSymbol* otherGenericInfo = static_cast<const GenericTypeSymbol*>(right);

			if (!AreTypesEqual(thisGenericInfo->UnderlayingType, otherGenericInfo->UnderlayingType))
				return false;

			const TypeSymbol* thisUnderlyingType = thisGenericInfo->UnderlayingType;
			const TypeSymbol* otherUnderlyingType = otherGenericInfo->UnderlayingType;

			std::size_t size = thisUnderlyingType->TypeParameters.size();
			if (otherUnderlyingType->TypeParameters.size() != size)
				return false;

			for (std::size_t i = 0; i < size; i++)
			{
				TypeSymbol* thisParam = const_cast<GenericTypeSymbol*>(thisGenericInfo)->SubstituteTypeParameters(thisUnderlyingType->TypeParameters.at(i));
				TypeSymbol* otherParam = const_cast<GenericTypeSymbol*>(otherGenericInfo)->SubstituteTypeParameters(otherUnderlyingType->TypeParameters.at(i));

				if (!AreTypesEqual(thisParam, otherParam))
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

			if (!AreTypesEqual(thisLambdaInfo->ReturnType, otherLambdaInfo->ReturnType))
				return false;

			std::size_t size = thisLambdaInfo->Parameters.size();
			if (otherLambdaInfo->Parameters.size() != size)
				return false;

			for (std::size_t i = 0; i < size; i++)
			{
				const ParameterSymbol* thisParam = thisLambdaInfo->Parameters.at(i);
				const ParameterSymbol* otherParam = otherLambdaInfo->Parameters.at(i);

				if (!AreTypesEqual(thisParam->Type, otherParam->Type))
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

			return AreTypesEqual(thisArrayInfo->UnderlayingType, otherArrayInfo->UnderlayingType);
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

			if (!SemanticModel::AreTypesEqual(targetArg, sourceArg))
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
			if (SemanticModel::AreTypesEqual(targetArg, elementType))
				return true;
		}
	}

	return false;
}

bool SemanticModel::IsAssignableTo(const TypeSymbol* target, const TypeSymbol* source)
{
	if (target == nullptr || source == nullptr)
		return false;

	if (AreTypesEqual(target, source))
		return true;

	if (source == SymbolTable::Primitives::Null)
		return target->Inlining == TypeInlining::ByReference || target == SymbolTable::Primitives::Any;

	if (target->Kind == SyntaxKind::InterfaceDeclaration)
	{
		for (TypeSymbol* iface : source->Interfaces)
		{
			if (AreTypesEqual(target, iface))
				return true;
		}

		if (source->Kind == SyntaxKind::ArrayType)
		{
			for (TypeSymbol* iface : SymbolTable::Primitives::Array->Interfaces)
			{
				if (AreTypesEqual(target, iface))
					return true;
			}
		}
	}

	if (target->Kind == SyntaxKind::GenericType)
	{
		GenericTypeSymbol* targetGeneric = const_cast<GenericTypeSymbol*>(static_cast<const GenericTypeSymbol*>(target));
		if (IsConstructedInterfaceImplemented(targetGeneric, const_cast<TypeSymbol*>(source)))
			return true;

		// Allow assigning a concrete delegate to a constructed generic delegate type
		// (e.g. lambda to Transform<int, int>).
		if (targetGeneric->UnderlayingType->Kind == SyntaxKind::DelegateType && source->Kind == SyntaxKind::DelegateType)
		{
			const DelegateTypeSymbol* targetDelegate = static_cast<const DelegateTypeSymbol*>(targetGeneric->UnderlayingType);
			const DelegateTypeSymbol* sourceDelegate = static_cast<const DelegateTypeSymbol*>(source);

			if (targetDelegate->ReturnType == nullptr || sourceDelegate->ReturnType == nullptr)
				return false;

			TypeSymbol* targetReturn = targetDelegate->ReturnType;
			if (targetReturn->Kind == SyntaxKind::TypeParameter)
			{
				TypeSymbol* substituted = targetGeneric->SubstituteTypeParameters(static_cast<TypeParameterSymbol*>(targetReturn));
				if (substituted != nullptr)
					targetReturn = substituted;
			}

			if (!AreTypesEqual(targetReturn, sourceDelegate->ReturnType))
				return false;

			if (targetDelegate->Parameters.size() != sourceDelegate->Parameters.size())
				return false;

			for (std::size_t i = 0; i < targetDelegate->Parameters.size(); ++i)
			{
				TypeSymbol* targetParam = targetDelegate->Parameters[i]->Type;
				if (targetParam->Kind == SyntaxKind::TypeParameter)
				{
					TypeSymbol* substituted = targetGeneric->SubstituteTypeParameters(static_cast<TypeParameterSymbol*>(targetParam));
					if (substituted != nullptr)
						targetParam = substituted;
				}

				if (!AreTypesEqual(targetParam, sourceDelegate->Parameters[i]->Type))
					return false;
			}

			return true;
		}
	}

	if (target->Kind == SyntaxKind::DelegateType && source->Kind == SyntaxKind::DelegateType)
	{
		const DelegateTypeSymbol* targetDelegate = static_cast<const DelegateTypeSymbol*>(target);
		const DelegateTypeSymbol* sourceDelegate = static_cast<const DelegateTypeSymbol*>(source);

		if (!AreTypesEqual(targetDelegate->ReturnType, sourceDelegate->ReturnType))
			return false;

		if (targetDelegate->Parameters.size() != sourceDelegate->Parameters.size())
			return false;

		for (std::size_t i = 0; i < targetDelegate->Parameters.size(); ++i)
		{
			if (!AreTypesEqual(targetDelegate->Parameters[i]->Type, sourceDelegate->Parameters[i]->Type))
				return false;
		}

		return true;
	}

	return false;
}

std::wstring SemanticModel::GetTypeDisplayName(const TypeSymbol* type)
{
	if (type == nullptr)
		return L"<error>";

	if (type->Kind == SyntaxKind::GenericType)
	{
		const GenericTypeSymbol* generic = static_cast<const GenericTypeSymbol*>(type);
		const TypeSymbol* underlying = generic->UnderlayingType;

		std::wstring result = underlying != nullptr ? underlying->Name : L"<error>";
		result += L"<";

		if (underlying != nullptr)
		{
			bool first = true;
			for (TypeParameterSymbol* param : underlying->TypeParameters)
			{
				if (!first)
					result += L", ";

				TypeSymbol* arg = const_cast<GenericTypeSymbol*>(generic)->SubstituteTypeParameters(param);
				result += GetTypeDisplayName(arg != nullptr ? arg : param);
				first = false;
			}
		}

		result += L">";
		return result;
	}

	if (type->Kind == SyntaxKind::ArrayType)
	{
		const ArrayTypeSymbol* array = static_cast<const ArrayTypeSymbol*>(type);
		return GetTypeDisplayName(array->UnderlayingType) + L"[]";
	}

	return type->Name;
}

bool SemanticModel::IsPrimitiveType(const TypeSymbol* type)
{
	return type == SymbolTable::Primitives::Boolean
		|| type == SymbolTable::Primitives::Integer
		|| type == SymbolTable::Primitives::Double
		|| type == SymbolTable::Primitives::Char
		|| type == SymbolTable::Primitives::String
		|| type == SymbolTable::Primitives::Array
		|| type == SymbolTable::Primitives::NativeInteger;
}

TypeSymbol* SemanticModel::SubstituteType(TypeSymbol* type, GenericTypeSymbol* within)
{
	if (type == nullptr)
		return nullptr;

	switch (type->Kind)
	{
		default:
			throw std::runtime_error("unknown syntax kind to substitute");

		case SyntaxKind::StructDeclaration:
		case SyntaxKind::ClassDeclaration:
		case SyntaxKind::InterfaceDeclaration:
		case SyntaxKind::DelegateType:
			return type;

		case SyntaxKind::ArrayType:
		{
			ArrayTypeSymbol* arrayType = static_cast<ArrayTypeSymbol*>(type);
			TypeSymbol* underlayingType = arrayType->UnderlayingType;

			if (underlayingType == nullptr)
				throw std::runtime_error("Cannot resolve underlaying type of array");

			if (underlayingType->Kind == SyntaxKind::TypeParameter)
			{
				if (within == nullptr)
					return type;

				underlayingType = within->SubstituteTypeParameters(static_cast<TypeParameterSymbol*>(underlayingType));
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
			if (within == nullptr)
				return type;

			TypeSymbol* resolvedType = within->SubstituteTypeParameters(static_cast<TypeParameterSymbol*>(type));
			return resolvedType != nullptr ? resolvedType : type;
		}
	}
}

TypeSymbol* SemanticModel::GetFieldType(FieldSymbol* field, GenericTypeSymbol* within)
{
	if (field == nullptr || field->ReturnType == nullptr)
		return nullptr;

	return SubstituteType(field->ReturnType, within);
}

TypeSymbol* SemanticModel::GetMethodReturnType(MethodSymbol* method, GenericTypeSymbol* within)
{
	if (method == nullptr || method->ReturnType == nullptr)
		return nullptr;

	return SubstituteType(method->ReturnType, within);
}

TypeSymbol* SemanticModel::GetPropertyType(PropertySymbol* property, GenericTypeSymbol* within)
{
	if (property == nullptr || property->ReturnType == nullptr)
		return nullptr;

	return SubstituteType(property->ReturnType, within);
}

TypeSymbol* SemanticModel::GetConstructorReturnType(ConstructorSymbol* constructor, GenericTypeSymbol* within)
{
	if (constructor == nullptr || constructor->ReturnType == nullptr)
		return nullptr;

	return SubstituteType(constructor->ReturnType, within);
}

// =========================================================================
//  Runtime generic argument resolution
// =========================================================================

TypeSymbol* SemanticModel::ResolveRuntimeTypeArgument(TypeSymbol* type, const TypeParameterResolver& resolveParameter)
{
	if (type == nullptr)
		return type;

	if (type->Kind == SyntaxKind::TypeParameter)
	{
		TypeSymbol* resolved = resolveParameter(static_cast<TypeParameterSymbol*>(type));
		return resolved != nullptr ? resolved : type;
	}

	if (type->Kind == SyntaxKind::ArrayType)
	{
		ArrayTypeSymbol* arrayType = static_cast<ArrayTypeSymbol*>(type);
		TypeSymbol* resolvedUnderlying = ResolveRuntimeTypeArgument(arrayType->UnderlayingType, resolveParameter);
		if (resolvedUnderlying == nullptr || resolvedUnderlying == arrayType->UnderlayingType)
			return type;

		ArrayTypeSymbol* resolvedArray = new ArrayTypeSymbol(resolvedUnderlying);
		resolvedArray->Length = arrayType->Length;
		resolvedArray->LayoutingState = arrayType->LayoutingState;
		return resolvedArray;
	}

	return type;
}

bool SemanticModel::TryResolveGenericArguments(TypeSymbol* type, const TypeParameterResolver& resolveParameter, TypeSymbol*& baseType, std::vector<TypeSymbol*>& genericArgs)
{
	if (type == nullptr)
		return false;

	if (type->Kind == SyntaxKind::TypeParameter)
	{
		TypeSymbol* resolved = resolveParameter(static_cast<TypeParameterSymbol*>(type));
		if (resolved != nullptr && resolved != type)
			return TryResolveGenericArguments(resolved, resolveParameter, baseType, genericArgs);

		return false;
	}

	if (type->Kind == SyntaxKind::GenericType)
	{
		GenericTypeSymbol* generic = static_cast<GenericTypeSymbol*>(type);
		baseType = generic->UnderlayingType;
		genericArgs.clear();
		genericArgs.reserve(baseType->TypeParameters.size());
		for (TypeParameterSymbol* param : baseType->TypeParameters)
		{
			TypeSymbol* arg = generic->SubstituteTypeParameters(param);
			TypeSymbol* resolvedArg = ResolveRuntimeTypeArgument(arg, resolveParameter);
			genericArgs.push_back(resolvedArg != nullptr ? resolvedArg : arg);
		}

		return true;
	}

	return false;
}
