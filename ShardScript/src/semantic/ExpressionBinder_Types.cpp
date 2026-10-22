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
	bool is_scalar_primitive(gmt::Ref<StructSymbol> type, const SymbolTable& table)
	{
		if (type.is_null())
			return false;

		if (type == table.get_boolean_primitive() || type == table.get_integer_primitive())
			return true;

		if (type == table.get_double_primitive() || type == table.get_char_primitive())
			return true;

		return type == table.get_byte_primitive() || type == table.get_nint_primitive();
	}

	bool is_reference_kind(gmt::Ref<StructSymbol> type)
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

	bool is_reference_cast_allowed(gmt::Ref<StructSymbol> source, gmt::Ref<StructSymbol> target, SemanticModel& model)
	{
		if (source.is_null() || target.is_null())
			return false;

		if (SemanticModel::are_types_equal(source, target))
			return true;

		if (!is_reference_kind(source) || !is_reference_kind(target))
			return false;

		if (model.is_assignable_to(target, source) || model.is_assignable_to(source, target))
			return true;

		return target->get_kind() == SyntaxKind::InterfaceDeclaration || source->get_kind() == SyntaxKind::InterfaceDeclaration;
	}

	gmt::Ref<OperatorSymbol> find_conversion_operator(gmt::Ref<StructSymbol> ownerType, gmt::Ref<StructSymbol> sourceType, gmt::Ref<StructSymbol> targetType)
	{
		if (ownerType.is_null() || sourceType.is_null() || targetType.is_null())
			return gmt::nullref;

		for (gmt::Ref<OperatorSymbol> candidate : ownerType->operators)
		{
			if (candidate.is_null() || candidate->get_operator_token() != TokenType::AsOperator)
				continue;

			const gmt::vector<gmt::Ref<ParameterSymbol>>& parameters = candidate->get_parameters();
			bool noParameter = parameters.empty();
			bool sourceParameter = parameters.size() == 1 && SemanticModel::are_types_equal(parameters[0]->get_type(), sourceType);
			if (!noParameter && !sourceParameter)
				continue;

			if (SemanticModel::are_types_equal(candidate->get_return_type(), targetType))
				return candidate;
		}

		return gmt::nullref;
	}
}

gmt::Ref<StructSymbol> ExpressionBinder::resolve_type_expression(gmt::Ref<const TypeSyntax> type)
{
	if (type.is_null())
		return gmt::nullref;

	// type syntaxes were resolved and bound by the type binder already
	return lookup_symbol<StructSymbol>(type);
}

gmt::Ref<StructSymbol> ExpressionBinder::substitute_type_parameters(gmt::Ref<StructSymbol> type, gmt::Ref<GenericTypeSymbol> genericType)
{
	if (type.is_null() || genericType.is_null() || type->get_kind() != SyntaxKind::TypeParameter)
		return type;

	gmt::Ref<StructSymbol> substituted = genericType->substitute_type_parameter(type.as<TypeParameterSymbol>());
	return !substituted.is_null() ? substituted : type;
}

gmt::Ref<StructSymbol> ExpressionBinder::substitute_method_type_parameter(gmt::Ref<StructSymbol> type, gmt::Ref<MethodSymbol> method, std::span<const gmt::Ref<StructSymbol>> methodTypeArgs)
{
	if (type.is_null() || method.is_null() || type->get_kind() != SyntaxKind::TypeParameter)
		return type;

	const gmt::vector<gmt::Ref<TypeParameterSymbol>>& typeParameters = method->get_type_parameters();
	for (std::size_t i = 0; i < typeParameters.size() && i < methodTypeArgs.size(); i++)
	{
		if (typeParameters[i] == type)
			return methodTypeArgs[i];
	}

	return type;
}

gmt::Ref<StructSymbol> ExpressionBinder::substitute_type_parameters(gmt::Ref<StructSymbol> type, gmt::Ref<GenericTypeSymbol> genericType, gmt::Ref<MethodSymbol> method, std::span<const gmt::Ref<StructSymbol>> methodTypeArgs)
{
	if (type.is_null())
		return gmt::nullref;

	if (type->get_kind() == SyntaxKind::TypeParameter)
	{
		gmt::Ref<StructSymbol> substituted = substitute_type_parameters(type, genericType);
		if (!substituted.is_null() && substituted->get_kind() == SyntaxKind::TypeParameter)
			substituted = substitute_method_type_parameter(substituted, method, methodTypeArgs);

		return !substituted.is_null() ? substituted : type;
	}

	if (type->get_kind() == SyntaxKind::GenericType)
	{
		gmt::Ref<GenericTypeSymbol> generic = type.as<GenericTypeSymbol>();
		gmt::Ref<StructSymbol> underlaying = generic->get_underlaying_type();

		bool changed = false;
		gmt::inline_vector<gmt::Ref<StructSymbol>, 4> newArguments;
		newArguments.get_vector().reserve(underlaying->type_parameters.size());
		for (gmt::Ref<TypeParameterSymbol> parameter : underlaying->type_parameters)
		{
			gmt::Ref<StructSymbol> argument = generic->substitute_type_parameter(parameter);
			gmt::Ref<StructSymbol> newArgument = substitute_type_parameters(argument, genericType, method, methodTypeArgs);
			if (newArgument != argument)
				changed = true;

			newArguments.push_back(!newArgument.is_null() ? newArgument : argument);
		}

		if (!changed)
			return type;

		return m_factory.create_generic_type(underlaying, newArguments.get_vector());
	}

	if (type->get_kind() == SyntaxKind::ArrayType)
	{
		gmt::Ref<ArrayTypeSymbol> array = type.as<ArrayTypeSymbol>();
		gmt::Ref<StructSymbol> elementType = substitute_type_parameters(array->get_underlaying_type(), genericType, method, methodTypeArgs);
		if (elementType == array->get_underlaying_type())
			return type;

		return m_factory.create_array_type(elementType);
	}

	if (type->get_kind() == SyntaxKind::NullableType)
	{
		gmt::Ref<NullableTypeSymbol> nullable = type.as<NullableTypeSymbol>();
		gmt::Ref<StructSymbol> underlaying = substitute_type_parameters(nullable->get_underlaying_type(), genericType, method, methodTypeArgs);
		if (underlaying == nullable->get_underlaying_type())
			return type;

		return m_factory.create_nullable_type(underlaying);
	}

	return type;
}

void ExpressionBinder::visit_cast_expression(const CastExpressionSyntax* node)
{
	visit_expression(node->get_expression());

	std::optional<gmt::Ref<const CastExpressionSyntax>> nodeRef = ref_from(node);
	gmt::Ref<StructSymbol> targetType = resolve_type_expression(node->get_target_type());
	if (targetType.is_null())
	{
		if (nodeRef.has_value())
			set_expression_type(*nodeRef, gmt::nullref);

		return;
	}

	gmt::Ref<StructSymbol> sourceType = get_expression_type(node->get_expression());
	if (sourceType.is_null() || SemanticModel::are_types_equal(sourceType, targetType))
	{
		if (nodeRef.has_value())
			set_expression_type(*nodeRef, targetType);

		return;
	}

	const SymbolTable& table = get_symbol_table();

	// 1. user-defined conversion operators
	gmt::Ref<OperatorSymbol> sourceOperator = find_conversion_operator(sourceType, sourceType, targetType);
	gmt::Ref<OperatorSymbol> targetOperator = find_conversion_operator(targetType, sourceType, targetType);

	int conversionCount = 0;
	if (!sourceOperator.is_null())
		conversionCount++;

	if (!targetOperator.is_null() && targetOperator != sourceOperator)
		conversionCount++;

	if (conversionCount > 1)
	{
		get_diagnostics().report_error(node->get_operator_token(), gmt::concat(L"Ambiguous cast from '", get_model().get_type_display_name(sourceType), L"' to '", get_model().get_type_display_name(targetType), L"'. Multiple user-defined 'operator as' conversions are available"));
		if (nodeRef.has_value())
			set_expression_type(*nodeRef, targetType);

		return;
	}

	if (!sourceOperator.is_null() || !targetOperator.is_null())
	{
		if (nodeRef.has_value())
		{
			gmt::Ref<OperatorSymbol> conversion = !sourceOperator.is_null() ? sourceOperator : targetOperator;
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

	std::optional<gmt::Ref<const IsExpressionSyntax>> nodeRef = ref_from(node);
	if (nodeRef.has_value())
		set_expression_type(*nodeRef, get_symbol_table().get_boolean_primitive());
}
