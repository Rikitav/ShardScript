#include <shard/semantic/ExpressionBinder.hpp>

#include <shard/parsing/SyntaxKind.hpp>
#include <shard/lexical/SyntaxFacts.hpp>

#include <shard/semantic/SemanticModel.hpp>
#include <shard/semantic/SymbolTable.hpp>
#include <shard/semantic/symbols/ArrayTypeSymbol.hpp>
#include <shard/semantic/symbols/GenericTypeSymbol.hpp>
#include <shard/semantic/symbols/NullableTypeSymbol.hpp>
#include <shard/semantic/symbols/OperatorSymbol.hpp>
#include <shard/semantic/symbols/ParameterSymbol.hpp>
#include <shard/semantic/symbols/StructSymbol.hpp>
#include <shard/semantic/symbols/TypeParameterSymbol.hpp>

#include <gmt/Concat.hpp>
#include <gmt/Containers.hpp>
#include <gmt/InlineVector.hpp>

#include <vector>

using namespace shard;

namespace
{
	bool is_scalar_primitive(StructSymbol* type, const SymbolTable& table)
	{
		if (type == nullptr)
			return false;

		if (type == table.get_boolean_primitive() || type == table.get_integer_primitive())
			return true;

		if (type == table.get_double_primitive() || type == table.get_char_primitive())
			return true;

		return type == table.get_byte_primitive() || type == table.get_nint_primitive();
	}

	bool is_reference_kind(StructSymbol* type)
	{
		switch (type->get_kind())
		{
			case SyntaxKind::ArrayType:
			case SyntaxKind::DelegateType:
			case SyntaxKind::InterfaceDeclaration:
			case SyntaxKind::NullableType:
				return true;

			default:
				return false;
		}
	}

	bool is_reference_cast_allowed(StructSymbol* source, StructSymbol* target, SemanticModel& model)
	{
		if ((source == nullptr) || (target == nullptr))
			return false;

		if (SemanticModel::are_types_equal(source, target))
			return true;

		if (!is_reference_kind(source) || !is_reference_kind(target))
			return false;

		if (model.is_assignable_to(target, source) || model.is_assignable_to(source, target))
			return true;

		return target->get_kind() == SyntaxKind::InterfaceDeclaration || source->get_kind() == SyntaxKind::InterfaceDeclaration;
	}

	OperatorSymbol* find_conversion_operator(StructSymbol* ownerType, StructSymbol* sourceType, StructSymbol* targetType)
	{
		if ((ownerType == nullptr) || (sourceType == nullptr) || (targetType == nullptr))
			return nullptr;

		for (OperatorSymbol* candidate : ownerType->operators)
		{
			if ((candidate == nullptr) || candidate->get_operator_token() != TokenType::AsOperator)
				continue;

			const gmt::vector<ParameterSymbol*>& parameters = candidate->get_parameters();
			bool noParameter = parameters.empty();
			bool sourceParameter = parameters.size() == 1 && SemanticModel::are_types_equal(parameters[0]->get_type(), sourceType);
			if (!noParameter && !sourceParameter)
				continue;

			if (SemanticModel::are_types_equal(candidate->get_return_type(), targetType))
				return candidate;
		}

		return nullptr;
	}
}

StructSymbol* ExpressionBinder::resolve_type_expression(const TypeSyntax* type)
{
	if (type == nullptr)
		return nullptr;

	// type syntaxes were resolved and bound by the type binder already
	return lookup_symbol<StructSymbol>(type);
}

StructSymbol* ExpressionBinder::substitute_type_parameters(StructSymbol* type, GenericTypeSymbol* genericType)
{
	if ((type == nullptr) || (genericType == nullptr))
		return type;

	// recursive: properties and fields may mention nested constructions such
	// as 'Task<T>' whose argument is the receiver's type parameter
	return get_model().substitute_type(type, genericType);
}

StructSymbol* ExpressionBinder::substitute_method_type_parameter(StructSymbol* type, MethodSymbol* method, std::span<StructSymbol* const> methodTypeArgs)
{
	if ((type == nullptr) || (method == nullptr) || type->get_kind() != SyntaxKind::TypeParameter)
		return type;

	const gmt::vector<TypeParameterSymbol*>& typeParameters = method->get_type_parameters();
	for (std::size_t i = 0; i < typeParameters.size() && i < methodTypeArgs.size(); i++)
	{
		if (typeParameters[i] == type)
			return methodTypeArgs[i];
	}

	return type;
}

StructSymbol* ExpressionBinder::substitute_type_parameters(StructSymbol* type, GenericTypeSymbol* genericType, MethodSymbol* method, std::span<StructSymbol* const> methodTypeArgs)
{
	if (type == nullptr)
		return nullptr;

	if (type->get_kind() == SyntaxKind::TypeParameter)
	{
		StructSymbol* substituted = substitute_type_parameters(type, genericType);
		if (!(substituted == nullptr) && substituted->get_kind() == SyntaxKind::TypeParameter)
			substituted = substitute_method_type_parameter(substituted, method, methodTypeArgs);

		return !(substituted == nullptr) ? substituted : type;
	}

	if (type->get_kind() == SyntaxKind::GenericType)
	{
		GenericTypeSymbol* generic = static_cast<GenericTypeSymbol*>(type);
		StructSymbol* underlaying = generic->get_underlaying_type();

		bool changed = false;
		gmt::inline_vector<StructSymbol*, 4> newArguments;
		newArguments.get_vector().reserve(underlaying->type_parameters.size());
		for (TypeParameterSymbol* parameter : underlaying->type_parameters)
		{
			StructSymbol* argument = generic->substitute_type_parameter(parameter);
			StructSymbol* newArgument = substitute_type_parameters(argument, genericType, method, methodTypeArgs);
			if (newArgument != argument)
				changed = true;

			newArguments.push_back(!(newArgument == nullptr) ? newArgument : argument);
		}

		if (!changed)
			return type;

		return m_factory.create_generic_type(underlaying, newArguments.get_vector());
	}

	if (type->get_kind() == SyntaxKind::ArrayType)
	{
		ArrayTypeSymbol* array = static_cast<ArrayTypeSymbol*>(type);
		StructSymbol* elementType = substitute_type_parameters(array->get_underlaying_type(), genericType, method, methodTypeArgs);
		if (elementType == array->get_underlaying_type())
			return type;

		return m_factory.create_array_type(elementType);
	}

	if (type->get_kind() == SyntaxKind::NullableType)
	{
		NullableTypeSymbol* nullable = static_cast<NullableTypeSymbol*>(type);
		StructSymbol* underlaying = substitute_type_parameters(nullable->get_underlaying_type(), genericType, method, methodTypeArgs);
		if (underlaying == nullable->get_underlaying_type())
			return type;

		return m_factory.create_nullable_type(underlaying);
	}

	return type;
}

void ExpressionBinder::visit_cast_expression(const CastExpressionSyntax* node)
{
	visit_expression(node->get_expression());

	std::optional<const CastExpressionSyntax*> nodeRef = ref_from(node);
	StructSymbol* targetType = resolve_type_expression(node->get_target_type());
	if (targetType == nullptr)
	{
		if (nodeRef.has_value())
			set_expression_type(*nodeRef, nullptr);

		return;
	}

	StructSymbol* sourceType = get_expression_type(node->get_expression());
	if ((sourceType == nullptr) || SemanticModel::are_types_equal(sourceType, targetType))
	{
		if (nodeRef.has_value())
			set_expression_type(*nodeRef, targetType);

		return;
	}

	const SymbolTable& table = get_symbol_table();

	// 1. user-defined conversion operators
	OperatorSymbol* sourceOperator = find_conversion_operator(sourceType, sourceType, targetType);
	OperatorSymbol* targetOperator = find_conversion_operator(targetType, sourceType, targetType);

	int conversionCount = 0;
	if (!(sourceOperator == nullptr))
		conversionCount++;

	if (!(targetOperator == nullptr) && targetOperator != sourceOperator)
		conversionCount++;

	if (conversionCount > 1)
	{
		get_diagnostics().report_error(node->get_operator_token(), gmt::concat(L"Ambiguous cast from '", get_model().get_type_display_name(sourceType), L"' to '", get_model().get_type_display_name(targetType), L"'. Multiple user-defined 'operator as' conversions are available"));
		if (nodeRef.has_value())
			set_expression_type(*nodeRef, targetType);

		return;
	}

	if (!(sourceOperator == nullptr) || !(targetOperator == nullptr))
	{
		if (nodeRef.has_value())
		{
			OperatorSymbol* conversion = !(sourceOperator == nullptr) ? sourceOperator : targetOperator;
			get_symbol_table().bind_symbol<OperatorSymbol>(*nodeRef, conversion);
			set_expression_type(*nodeRef, targetType);
		}

		return;
	}

	// 2. built-in reference casts
	if (is_reference_cast_allowed(sourceType, targetType, get_model()))
	{
		if (nodeRef.has_value())
			set_expression_type(*nodeRef, targetType);

		return;
	}

	// 3. built-in primitive conversions
	bool bothScalar = is_scalar_primitive(sourceType, table) && is_scalar_primitive(targetType, table);
	if (bothScalar)
	{
		if (nodeRef.has_value())
			set_expression_type(*nodeRef, targetType);

		return;
	}

	get_diagnostics().report_error(node->get_operator_token(), gmt::concat(L"Cannot cast from '", get_model().get_type_display_name(sourceType), L"' to '", get_model().get_type_display_name(targetType), L"'"));
	if (nodeRef.has_value())
		set_expression_type(*nodeRef, targetType);
}

void ExpressionBinder::visit_is_expression(const IsExpressionSyntax* node)
{
	visit_expression(node->get_expression());
	resolve_type_expression(node->get_target_type());

	std::optional<const IsExpressionSyntax*> nodeRef = ref_from(node);
	if (nodeRef.has_value())
		set_expression_type(*nodeRef, get_symbol_table().get_boolean_primitive());
}
