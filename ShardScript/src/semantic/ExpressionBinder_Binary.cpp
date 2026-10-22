#include <shard/semantic/ExpressionBinder.hpp>

#include <shard/lexical/SyntaxFacts.hpp>
#include <shard/parsing/SyntaxKind.hpp>

#include <shard/semantic/SemanticModel.hpp>
#include <shard/semantic/SemanticScope.hpp>
#include <shard/semantic/symbols/EnumSymbol.hpp>
#include <shard/semantic/symbols/FieldSymbol.hpp>
#include <shard/semantic/symbols/LeftDenotationSymbol.hpp>
#include <shard/semantic/symbols/OperatorSymbol.hpp>
#include <shard/semantic/symbols/ParameterSymbol.hpp>
#include <shard/semantic/symbols/StructSymbol.hpp>

#include <gmt/Concat.hpp>
#include <gmt/Containers.hpp>

#include <span>
#include <vector>

using namespace shard;

namespace
{
	gmt::Ref<OperatorSymbol> find_operator(gmt::Ref<StructSymbol> owner, TokenType operatorToken, std::span<const gmt::Ref<StructSymbol>> parameterTypes)
	{
		if (owner.is_null())
			return gmt::nullref;

		for (gmt::Ref<OperatorSymbol> candidate : owner->operators)
		{
			if (candidate.is_null() || candidate->get_operator_token() != operatorToken)
				continue;

			const gmt::vector<gmt::Ref<ParameterSymbol>>& parameters = candidate->get_parameters();
			if (parameters.size() != parameterTypes.size())
				continue;

			bool parametersMatch = true;
			for (std::size_t i = 0; i < parameters.size(); i++)
			{
				if (!SemanticModel::are_types_equal(parameters[i]->get_type(), parameterTypes[i]))
				{
					parametersMatch = false;
					break;
				}
			}

			if (parametersMatch)
				return candidate;
		}

		return gmt::nullref;
	}

	static bool is_numeric_primitive(gmt::Ref<StructSymbol> type, const SymbolTable& table)
	{
		if (type == table.get_integer_primitive())
			return true;

		if (type == table.get_double_primitive())
			return true;

		if (type == table.get_char_primitive())
			return true;

		if (type == table.get_byte_primitive())
			return true;

		return type == table.get_nint_primitive();
	}

	static bool is_integral_primitive(gmt::Ref<StructSymbol> type, const SymbolTable& table)
	{
		if (type == table.get_integer_primitive())
			return true;

		if (type == table.get_char_primitive())
			return true;

		if (type == table.get_byte_primitive())
			return true;

		return type == table.get_nint_primitive();
	}

	static bool are_types_comparable(gmt::Ref<StructSymbol> leftType, gmt::Ref<StructSymbol> rightType, bool bothTypeParameters, const SymbolTable& table, const SemanticModel& model)
	{
		if (leftType.is_null() || rightType.is_null())
			return true;

		if (bothTypeParameters)
			return true;

		if (leftType == table.get_string_primitive() && rightType == table.get_string_primitive())
			return true;

		if (is_numeric_primitive(leftType, table) && is_numeric_primitive(rightType, table))
			return true;

		if (leftType == table.get_boolean_primitive() && rightType == table.get_boolean_primitive())
			return true;

		bool bothEnums = leftType->get_kind() == SyntaxKind::EnumDeclaration && rightType->get_kind() == SyntaxKind::EnumDeclaration;
		if (bothEnums && leftType == rightType)
			return true;

		if (model.is_assignable_to(leftType, rightType))
			return true;

		return model.is_assignable_to(rightType, leftType);
	}
}

bool ExpressionBinder::is_assignment_operator(TokenType type)
{
	switch (type)
	{
		case TokenType::AssignOperator:
		case TokenType::AddAssignOperator:
		case TokenType::SubAssignOperator:
		case TokenType::MultAssignOperator:
		case TokenType::DivAssignOperator:
		case TokenType::ModAssignOperator:
		case TokenType::PowAssignOperator:
			return true;

		default:
			return false;
	}
}

bool ExpressionBinder::is_assignment_context(const IndexatorExpressionSyntax* expression)
{
	if (expression == nullptr)
		return false;

	gmt::Ref<const SyntaxNode> parent = expression->get_parent();
	if (parent.is_null() || parent->get_kind() != SyntaxKind::BinaryExpression)
		return false;

	gmt::Ref<const BinaryExpressionSyntax> binary = parent.as<BinaryExpressionSyntax>();
	if (!is_assignment_operator(binary->get_operator_token().get_type()))
		return false;

	return binary->get_left().as_ptr() == expression;
}

gmt::Ref<OperatorSymbol> ExpressionBinder::resolve_operator_method(gmt::Ref<StructSymbol> ownerType, TokenType operatorToken, std::span<const gmt::Ref<StructSymbol>> parameterTypes)
{
	if (ownerType.is_null() || !is_overloadable_operator(operatorToken))
		return gmt::nullref;

	gmt::Ref<StructSymbol> searchType = ownerType;
	if (searchType->get_kind() == SyntaxKind::GenericType)
		searchType = searchType.as<GenericTypeSymbol>()->get_underlaying_type();

	return find_operator(searchType, operatorToken, parameterTypes);
}

gmt::Ref<StructSymbol> ExpressionBinder::analyze_binary_expression(const BinaryExpressionSyntax* node)
{
	const TokenType op = node->get_operator_token().get_type();
	SymbolTable& table = get_symbol_table();
	SemanticModel& model = get_model();

	visit_expression(node->get_left());
	gmt::Ref<StructSymbol> leftType = get_expression_type(node->get_left());

	if (leftType.is_null() && op != TokenType::AssignOperator)
	{
		get_diagnostics().report_error(node->get_location(), L"Left operand type could not be determined");
		return gmt::nullref;
	}

	if (op == TokenType::AssignOperator)
	{
		gmt::Ref<LeftDenotationSymbol> denotation = table.emplace<LeftDenotationSymbol>(leftType, gmt::Ref<SyntaxSymbol>(gmt::nullref));
		push_scope(denotation);
		visit_expression(node->get_right());
		pop_scope();

		gmt::Ref<StructSymbol> rightType = get_expression_type(node->get_right());
		if (rightType.is_null())
		{
			get_diagnostics().report_error(node->get_location(), L"Right operand type could not be determined");
			return leftType;
		}

		if (!model.is_assignable_to(leftType, rightType))
		{
			get_diagnostics().report_error(node->get_location(), gmt::concat(L"Type mismatch in assignment: expected '", model.get_type_display_name(leftType), L"' but got '", model.get_type_display_name(rightType), L"'"));
			return leftType;
		}

		return leftType;
	}

	bool enumBitwiseContext = false;
	if (!leftType.is_null() && leftType->get_kind() == SyntaxKind::EnumDeclaration)
		enumBitwiseContext = op == TokenType::OrOperator || op == TokenType::AndOperator;

	if (enumBitwiseContext)
	{
		push_scope(gmt::nullref);
		for (gmt::Ref<FieldSymbol> field : leftType.as<EnumSymbol>()->fields)
		{
			if (!field.is_null() && field->get_is_enum_value())
				get_current_scope()->declare_symbol(field);
		}
	}

	visit_expression(node->get_right());

	if (enumBitwiseContext)
		pop_scope();

	gmt::Ref<StructSymbol> rightType = get_expression_type(node->get_right());

	if (rightType.is_null())
	{
		get_diagnostics().report_error(node->get_location(), L"Right operand type could not be determined");
		return leftType;
	}

	bool leftIsNull = leftType.is_null();
	bool rightIsNull = rightType.is_null();

	bool bothPrimitive = false;
	if (!leftIsNull && !rightIsNull)
		bothPrimitive = model.is_primitive_type(leftType) && model.is_primitive_type(rightType);

	bool bothTypeParameters = false;
	if (!leftIsNull && leftType->get_kind() == SyntaxKind::TypeParameter)
		bothTypeParameters = leftType == rightType;

	bool isPrimitiveBinaryOp = false;
	if (bothPrimitive || bothTypeParameters)
	{
		isPrimitiveBinaryOp = is_binary_arithmetic_operator(op)
			|| is_binary_boolean_operator(op)
			|| is_binary_bit_operator(op);
	}

	if (!isPrimitiveBinaryOp)
	{
		gmt::Ref<StructSymbol> parameterTypes[] = { leftType, rightType };
		gmt::Ref<OperatorSymbol> operatorMethod = resolve_operator_method(leftType, op, parameterTypes);
		if (!operatorMethod.is_null())
		{
			std::optional<gmt::Ref<const BinaryExpressionSyntax>> nodeRef = ref_from(node);
			if (nodeRef.has_value())
				table.bind_symbol<OperatorSymbol>(*nodeRef, operatorMethod);

			return operatorMethod->get_return_type();
		}

		bool leftIsNonPrimitive = !leftIsNull && !model.is_primitive_type(leftType);
		bool operatorIsEquality = op == TokenType::EqualsOperator || op == TokenType::NotEqualsOperator;
		if (leftIsNonPrimitive && !operatorIsEquality)
		{
			get_diagnostics().report_error(node->get_location(), gmt::concat(L"Operator '", get_operator_method_name(op), L"' is not defined for type '", leftType->get_name(), L"'"));
			return gmt::nullref;
		}
	}

	switch (op)
	{
		case TokenType::EqualsOperator:
		case TokenType::NotEqualsOperator:
		case TokenType::GreaterOperator:
		case TokenType::GreaterOrEqualsOperator:
		case TokenType::LessOperator:
		case TokenType::LessOrEqualsOperator:
		{
			bool comparable = are_types_comparable(leftType, rightType, bothTypeParameters, table, model);

			if (!comparable)
			{
				get_diagnostics().report_error(node->get_location(), gmt::concat(L"Type mismatch in comparison: '", leftType->get_name(), L"' and '", rightType->get_name(), L"'"));
			}

			return table.get_boolean_primitive();
		}

		case TokenType::OrOperator:
		case TokenType::AndOperator:
		{
			if (bothTypeParameters)
				return leftType;

			bool boolOp = leftType == table.get_boolean_primitive() && rightType == table.get_boolean_primitive();
			bool bitwiseOp = is_integral_primitive(leftType, table) && is_integral_primitive(rightType, table);

			if (!boolOp && !bitwiseOp)
			{
				get_diagnostics().report_error(node->get_location(), gmt::concat(L"Operator '", get_operator_method_name(op), L"' is not defined for type '", leftType->get_name(), L"'"));
				return gmt::nullref;
			}

			return boolOp ? table.get_boolean_primitive() : table.get_integer_primitive();
		}

		case TokenType::LeftShiftOperator:
		case TokenType::RightShiftOperator:
		{
			if (bothTypeParameters)
				return leftType;

			if (!is_integral_primitive(leftType, table) || !is_integral_primitive(rightType, table))
			{
				get_diagnostics().report_error(node->get_location(), gmt::concat(L"Operator '", get_operator_method_name(op), L"' is not defined for type '", leftType->get_name(), L"'"));
				return gmt::nullref;
			}

			return table.get_integer_primitive();
		}

		case TokenType::AddOperator:
		{
			if (bothTypeParameters)
				return leftType;

			if (leftType == table.get_string_primitive() || rightType == table.get_string_primitive())
				return table.get_string_primitive();

			if (!is_numeric_primitive(leftType, table) || !is_numeric_primitive(rightType, table))
			{
				get_diagnostics().report_error(node->get_location(), gmt::concat(L"Type mismatch in addition: '", leftType->get_name(), L"' and '", rightType->get_name(), L"'"));
				return gmt::nullref;
			}

			if (leftType == table.get_double_primitive() || rightType == table.get_double_primitive())
				return table.get_double_primitive();

			return table.get_integer_primitive();
		}

		case TokenType::SubOperator:
		case TokenType::MultOperator:
		case TokenType::DivOperator:
		case TokenType::ModOperator:
		case TokenType::PowOperator:
		{
			if (bothTypeParameters)
				return leftType;

			if (!is_numeric_primitive(leftType, table) || !is_numeric_primitive(rightType, table))
			{
				get_diagnostics().report_error(node->get_location(), gmt::concat(L"Type mismatch in arithmetic operation: '", leftType->get_name(), L"' and '", rightType->get_name(), L"'"));
				return gmt::nullref;
			}

			if (leftType == table.get_double_primitive() || rightType == table.get_double_primitive())
				return table.get_double_primitive();

			return table.get_integer_primitive();
		}

		case TokenType::AddAssignOperator:
		{
			if (leftType == table.get_string_primitive())
				return leftType;

			if (!is_numeric_primitive(leftType, table) || !is_numeric_primitive(rightType, table))
			{
				get_diagnostics().report_error(node->get_location(), gmt::concat(L"Type mismatch in addition assignment: '", leftType->get_name(), L"' and '", rightType->get_name(), L"'"));
				return gmt::nullref;
			}

			if (leftType == table.get_double_primitive() || rightType == table.get_double_primitive())
				return table.get_double_primitive();

			return table.get_integer_primitive();
		}

		case TokenType::SubAssignOperator:
		case TokenType::MultAssignOperator:
		case TokenType::DivAssignOperator:
		case TokenType::ModAssignOperator:
		case TokenType::PowAssignOperator:
		{
			if (!is_numeric_primitive(leftType, table) || !is_numeric_primitive(rightType, table))
			{
				get_diagnostics().report_error(node->get_location(), gmt::concat(L"Type mismatch in arithmetic assignment: '", leftType->get_name(), L"' and '", rightType->get_name(), L"'"));
				return gmt::nullref;
			}

			if (leftType == table.get_double_primitive() || rightType == table.get_double_primitive())
				return table.get_double_primitive();

			return table.get_integer_primitive();
		}

		default:
			return leftType;
	}
}

void ExpressionBinder::visit_binary_expression(const BinaryExpressionSyntax* node)
{
	gmt::Ref<StructSymbol> type = analyze_binary_expression(node);

	std::optional<gmt::Ref<const BinaryExpressionSyntax>> nodeRef = ref_from(node);
	if (nodeRef.has_value())
		set_expression_type(*nodeRef, type);
}

gmt::Ref<StructSymbol> ExpressionBinder::analyze_unary_expression(const UnaryExpressionSyntax* node)
{
	const TokenType op = node->get_operator_token().get_type();
	SymbolTable& table = get_symbol_table();

	gmt::Ref<StructSymbol> expressionType = get_expression_type(node->get_operand());
	if (expressionType.is_null())
	{
		get_diagnostics().report_error(node->get_location(), L"Operand type could not be determined");
		return gmt::nullref;
	}

	if (op == TokenType::TryKeyword)
		return expressionType;

	bool isIncrementOrDecrement = op == TokenType::IncrementOperator || op == TokenType::DecrementOperator;
	bool tryOperatorMethod = !get_model().is_primitive_type(expressionType) && !isIncrementOrDecrement;

	if (tryOperatorMethod)
	{
		gmt::Ref<StructSymbol> parameterTypes[] = { expressionType };
		gmt::Ref<OperatorSymbol> operatorMethod = resolve_operator_method(expressionType, op, parameterTypes);
		if (!operatorMethod.is_null())
		{
			std::optional<gmt::Ref<const UnaryExpressionSyntax>> nodeRef = ref_from(node);
			if (nodeRef.has_value())
				table.bind_symbol<OperatorSymbol>(*nodeRef, operatorMethod);

			return operatorMethod->get_return_type();
		}

		get_diagnostics().report_error(node->get_location(), gmt::concat(L"Operator '", get_operator_method_name(op), L"' is not defined for type '", expressionType->get_name(), L"'"));
		return gmt::nullref;
	}

	if (!get_model().is_primitive_type(expressionType))
	{
		get_diagnostics().report_error(node->get_location(), gmt::concat(L"Operator '", get_operator_method_name(op), L"' is not defined for type '", expressionType->get_name(), L"'"));
		return gmt::nullref;
	}

	switch (op)
	{
		case TokenType::NotOperator:
		{
			if (expressionType != table.get_boolean_primitive())
			{
				get_diagnostics().report_error(node->get_location(), gmt::concat(L"Logical NOT operator requires boolean type, got '", expressionType->get_name(), L"'"));
				return gmt::nullref;
			}

			return table.get_boolean_primitive();
		}

		case TokenType::SubOperator:
		case TokenType::AddOperator:
		case TokenType::IncrementOperator:
		case TokenType::DecrementOperator:
		{
			if (expressionType != table.get_integer_primitive() && expressionType != table.get_double_primitive())
			{
				get_diagnostics().report_error(node->get_location(), gmt::concat(L"Arithmetic unary operator requires numeric type, got '", expressionType->get_name(), L"'"));
				return gmt::nullref;
			}

			return expressionType;
		}

		default:
		{
			get_diagnostics().report_error(node->get_location(), L"Unknown unary operator");
			return gmt::nullref;
		}
	}
}

void ExpressionBinder::visit_unary_expression(const UnaryExpressionSyntax* node)
{
	visit_expression(node->get_operand());

	gmt::Ref<StructSymbol> type = analyze_unary_expression(node);

	std::optional<gmt::Ref<const UnaryExpressionSyntax>> nodeRef = ref_from(node);
	if (nodeRef.has_value())
		set_expression_type(*nodeRef, type);
}
