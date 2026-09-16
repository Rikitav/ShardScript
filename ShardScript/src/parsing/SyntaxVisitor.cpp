#include <shard/parsing/SyntaxVisitor.hpp>
#include <shard/parsing/SyntaxTree.hpp>

using namespace shard;

void SyntaxVisitor::visit_syntax_tree(const SyntaxTree& tree)
{
	for (const auto& unitReference : tree.get_units())
	{
		const TranslationUnitSyntax* unit = unitReference.as_ptr();
		visit_translation_unit(unit);

		for (const gmt::Ref<UsingDirectiveSyntax>& directive : unit->get_usings().as_span())
			visit_using_directive(directive.as_ptr());

		if (unit->has_namespace())
			visit_namespace_directive(unit->get_namespace().as_ptr());

		for (const gmt::Ref<MemberDeclarationSyntax>& member : unit->get_members().as_span())
			member.as_ptr()->accept(*this);
	}
}

void SyntaxVisitor::visit_translation_unit(const TranslationUnitSyntax* node)
{
	if (node == nullptr)
		return;
}

void SyntaxVisitor::visit_using_directive(const UsingDirectiveSyntax* node)
{
	if (node == nullptr)
		return;
}

void SyntaxVisitor::visit_namespace_directive(const NamespaceDirectiveSyntax* node)
{
	if (node == nullptr)
		return;
}

void SyntaxVisitor::visit_attribute(const AttributeSyntax* node)
{
	if (node == nullptr)
		return;
}

void SyntaxVisitor::visit_attributes_list(const AttributesListSyntax* node)
{
	if (node == nullptr)
		return;

	for (const gmt::Ref<AttributeSyntax>& attribute : node->get_attributes().as_span())
		visit_attribute(attribute.as_ptr());
}

void SyntaxVisitor::visit_class_declaration(const ClassDeclarationSyntax* node)
{
	if (node == nullptr)
		return;

	if (!node->get_attributes().is_null())
		visit_attributes_list(node->get_attributes().as_ptr());

	if (!node->get_type_parameters().is_null())
		visit_type_parameters_list(node->get_type_parameters().as_ptr());

	if (!node->get_where_clauses().is_null())
		visit_where_clauses_list(node->get_where_clauses().as_ptr());

	if (!node->get_base_types().is_null())
		visit_base_types_list(node->get_base_types().as_ptr());

	for (const gmt::Ref<MemberDeclarationSyntax>& member : node->get_members().as_span())
		member.as_ptr()->accept(*this);
}

void SyntaxVisitor::visit_function_declaration(const FunctionDeclarationSyntax* node)
{
	if (node == nullptr)
		return;

	if (!node->get_attributes().is_null())
		visit_attributes_list(node->get_attributes().as_ptr());

	if (!node->get_type_parameters().is_null())
		visit_type_parameters_list(node->get_type_parameters().as_ptr());

	if (!node->get_where_clauses().is_null())
		visit_where_clauses_list(node->get_where_clauses().as_ptr());

	if (!node->get_parameters_list().is_null())
		visit_parameters_list(node->get_parameters_list().as_ptr());

	if (!node->get_return_type().is_null())
		node->get_return_type().as_ptr()->accept(*this);

	if (!node->get_body().is_null())
		node->get_body().as_ptr()->accept(*this);
}

void SyntaxVisitor::visit_statements_block(const StatementsBlockSyntax* node)
{
	if (node == nullptr)
		return;

	for (const gmt::Ref<StatementSyntax>& statement : node->get_statements().as_span())
		statement.as_ptr()->accept(*this);
}

void SyntaxVisitor::visit_arrow_clause(const ArrowClauseSyntax* node)
{
	if (node == nullptr)
		return;

	if (!node->get_expression().is_null())
		node->get_expression().as_ptr()->accept(*this);
}

void SyntaxVisitor::visit_expression_statement(const ExpressionStatementSyntax* node)
{
	if (node == nullptr)
		return;

	if (!node->get_expression().is_null())
		node->get_expression().as_ptr()->accept(*this);
}

void SyntaxVisitor::visit_variable_statement(const VariableStatementSyntax* node)
{
	if (node == nullptr)
		return;

	if (!node->get_type().is_null())
		node->get_type().as_ptr()->accept(*this);

	if (!node->get_expression().is_null())
		node->get_expression().as_ptr()->accept(*this);
}

void SyntaxVisitor::visit_literal_expression(const LiteralExpressionSyntax* node)
{
	if (node == nullptr)
		return;
}

void SyntaxVisitor::visit_binary_expression(const BinaryExpressionSyntax* node)
{
	if (node == nullptr)
		return;

	if (!node->get_left().is_null())
		node->get_left().as_ptr()->accept(*this);

	if (!node->get_right().is_null())
		node->get_right().as_ptr()->accept(*this);
}

void SyntaxVisitor::visit_unary_expression(const UnaryExpressionSyntax* node)
{
	if (node == nullptr)
		return;

	if (!node->get_operand().is_null())
		node->get_operand().as_ptr()->accept(*this);
}

void SyntaxVisitor::visit_member_access_expression(const MemberAccessExpressionSyntax* node)
{
	if (node == nullptr)
		return;

	if (!node->get_previous().is_null())
		node->get_previous().as_ptr()->accept(*this);
}

void SyntaxVisitor::visit_invokation_expression(const InvokationExpressionSyntax* node)
{
	if (node == nullptr)
		return;

	if (!node->get_previous().is_null())
		node->get_previous().as_ptr()->accept(*this);

	if (!node->get_arguments().is_null())
		visit_arguments_list(node->get_arguments().as_ptr());
}

void SyntaxVisitor::visit_indexator_expression(const IndexatorExpressionSyntax* node)
{
	if (node == nullptr)
		return;

	if (!node->get_previous().is_null())
		node->get_previous().as_ptr()->accept(*this);

	if (!node->get_arguments().is_null())
		visit_arguments_list(node->get_arguments().as_ptr());
}

void SyntaxVisitor::visit_is_expression(const IsExpressionSyntax* node)
{
	if (node == nullptr)
		return;

	if (!node->get_expression().is_null())
		node->get_expression().as_ptr()->accept(*this);

	if (!node->get_target_type().is_null())
		node->get_target_type().as_ptr()->accept(*this);
}

void SyntaxVisitor::visit_cast_expression(const CastExpressionSyntax* node)
{
	if (node == nullptr)
		return;

	if (!node->get_expression().is_null())
		node->get_expression().as_ptr()->accept(*this);

	if (!node->get_target_type().is_null())
		node->get_target_type().as_ptr()->accept(*this);
}

void SyntaxVisitor::visit_if_expression(const IfExpressionSyntax* node)
{
	if (node == nullptr)
		return;

	if (!node->get_condition().is_null())
		node->get_condition().as_ptr()->accept(*this);

	if (!node->get_then_expression().is_null())
		node->get_then_expression().as_ptr()->accept(*this);

	if (!node->get_else_expression().is_null())
		node->get_else_expression().as_ptr()->accept(*this);
}

void SyntaxVisitor::visit_await_expression(const AwaitExpressionSyntax* node)
{
	if (node == nullptr)
		return;

	if (!node->get_expression().is_null())
		node->get_expression().as_ptr()->accept(*this);
}

void SyntaxVisitor::visit_range_expression(const RangeExpressionSyntax* node)
{
	if (node == nullptr)
		return;

	if (!node->get_left().is_null())
		node->get_left().as_ptr()->accept(*this);

	if (!node->get_right().is_null())
		node->get_right().as_ptr()->accept(*this);
}

void SyntaxVisitor::visit_collection_expression(const CollectionExpressionSyntax* node)
{
	if (node == nullptr)
		return;

	for (const gmt::Ref<ExpressionSyntax>& value : node->get_values().as_span())
		value.as_ptr()->accept(*this);
}

void SyntaxVisitor::visit_object_creation_expression(const ObjectExpressionSyntax* node)
{
	if (node == nullptr)
		return;

	if (!node->get_type().is_null())
		node->get_type().as_ptr()->accept(*this);

	if (!node->get_arguments().is_null())
		visit_arguments_list(node->get_arguments().as_ptr());

	if (!node->get_array_size().is_null())
		node->get_array_size().as_ptr()->accept(*this);
}

void SyntaxVisitor::visit_lambda_expression(const LambdaExpressionSyntax* node)
{
	if (node == nullptr)
		return;

	if (!node->get_parameters().is_null())
		visit_parameters_list(node->get_parameters().as_ptr());

	if (!node->get_body().is_null())
		node->get_body().as_ptr()->accept(*this);
}

void SyntaxVisitor::visit_parameter(const ParameterSyntax* node)
{
	if (node == nullptr)
		return;

	if (!node->get_type().is_null())
		node->get_type().as_ptr()->accept(*this);
}

void SyntaxVisitor::visit_parameters_list(const ParametersListSyntax* node)
{
	if (node == nullptr)
		return;

	for (const gmt::Ref<ParameterSyntax>& parameter : node->get_parameters().as_span())
		visit_parameter(parameter.as_ptr());
}

void SyntaxVisitor::visit_argument(const ArgumentSyntax* node)
{
	if (node == nullptr)
		return;

	if (!node->get_expression().is_null())
		node->get_expression().as_ptr()->accept(*this);
}

void SyntaxVisitor::visit_arguments_list(const ArgumentsListSyntax* node)
{
	if (node == nullptr)
		return;

	for (const gmt::Ref<ArgumentSyntax>& argument : node->get_arguments().as_span())
		visit_argument(argument.as_ptr());
}

void SyntaxVisitor::visit_type_arguments_list(const TypeArgumentsListSyntax* node)
{
	if (node == nullptr)
		return;

	for (const gmt::Ref<TypeSyntax>& type : node->get_types().as_span())
		type.as_ptr()->accept(*this);
}

void SyntaxVisitor::visit_type_parameter(const TypeParameterSyntax* node)
{
	if (node == nullptr)
		return;
}

void SyntaxVisitor::visit_type_parameters_list(const TypeParametersListSyntax* node)
{
	if (node == nullptr)
		return;

	for (const gmt::Ref<TypeParameterSyntax>& parameter : node->get_parameters().as_span())
		visit_type_parameter(parameter.as_ptr());
}

void SyntaxVisitor::visit_base_types_list(const BaseTypesListSyntax* node)
{
	if (node == nullptr)
		return;

	for (const gmt::Ref<TypeSyntax>& type : node->get_types().as_span())
		type.as_ptr()->accept(*this);
}

void SyntaxVisitor::visit_where_clause(const WhereClauseSyntax* node)
{
	if (node == nullptr)
		return;

	for (const gmt::Ref<TypeSyntax>& constraint : node->get_constraint_types().as_span())
		constraint.as_ptr()->accept(*this);
}

void SyntaxVisitor::visit_where_clauses_list(const WhereClausesListSyntax* node)
{
	if (node == nullptr)
		return;

	for (const gmt::Ref<WhereClauseSyntax>& clause : node->get_clauses().as_span())
		visit_where_clause(clause.as_ptr());
}

void SyntaxVisitor::visit_predefined_type(const PredefinedTypeSyntax* node)
{
	if (node == nullptr)
		return;
}

void SyntaxVisitor::visit_identifier_name_type(const IdentifierNameTypeSyntax* node)
{
	if (node == nullptr)
		return;
}

void SyntaxVisitor::visit_qualified_name_type(const QualifiedNameTypeSyntax* node)
{
	if (node == nullptr)
		return;

	if (!node->get_left().is_null())
		node->get_left().as_ptr()->accept(*this);
}

void SyntaxVisitor::visit_generic_type(const GenericTypeSyntax* node)
{
	if (node == nullptr)
		return;

	if (!node->get_underlaying_type().is_null())
		node->get_underlaying_type().as_ptr()->accept(*this);

	if (!node->get_type_arguments().is_null())
		visit_type_arguments_list(node->get_type_arguments().as_ptr());
}

void SyntaxVisitor::visit_array_type(const ArrayTypeSyntax* node)
{
	if (node == nullptr)
		return;

	if (!node->get_underlaying_type().is_null())
		node->get_underlaying_type().as_ptr()->accept(*this);
}

void SyntaxVisitor::visit_nullable_type(const NullableTypeSyntax* node)
{
	if (node == nullptr)
		return;

	if (!node->get_underlaying_type().is_null())
		node->get_underlaying_type().as_ptr()->accept(*this);
}
