#include <shard/parsing/SyntaxVisitor.hpp>
#include <shard/parsing/SyntaxTree.hpp>

using namespace shard;

void SyntaxVisitor::visit_syntax_tree(const SyntaxTree& tree)
{
	for (const auto& unitReference : tree.get_units())
		visit_translation_unit(unitReference);
}

void SyntaxVisitor::visit_translation_unit(const TranslationUnitSyntax* node)
{
	if (node == nullptr)
		return;

	for (UsingDirectiveSyntax* const& directive : node->get_usings())
		visit_using_directive(directive);

	if (node->has_namespace())
		visit_namespace_directive(node->get_namespace());

	for (MemberDeclarationSyntax* const& member : node->get_members())
		member->accept(*this);
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

	for (AttributeSyntax* const& attribute : node->get_attributes())
		visit_attribute(attribute);
}

void SyntaxVisitor::visit_struct_declaration(const StructDeclarationSyntax* node)
{
	if (node == nullptr)
		return;

	if (!(node->get_attributes() == nullptr))
		visit_attributes_list(node->get_attributes());

	if (!(node->get_type_parameters() == nullptr))
		visit_type_parameters_list(node->get_type_parameters());

	if (!(node->get_where_clauses() == nullptr))
		visit_where_clauses_list(node->get_where_clauses());

	if (!(node->get_base_types() == nullptr))
		visit_base_types_list(node->get_base_types());

	for (MemberDeclarationSyntax* const& member : node->get_members())
		member->accept(*this);
}

void SyntaxVisitor::visit_interface_declaration(const InterfaceDeclarationSyntax* node)
{
	if (node == nullptr)
		return;

	if (!(node->get_attributes() == nullptr))
		visit_attributes_list(node->get_attributes());

	if (!(node->get_type_parameters() == nullptr))
		visit_type_parameters_list(node->get_type_parameters());

	if (!(node->get_where_clauses() == nullptr))
		visit_where_clauses_list(node->get_where_clauses());

	if (!(node->get_base_types() == nullptr))
		visit_base_types_list(node->get_base_types());

	for (MemberDeclarationSyntax* const& member : node->get_members())
		member->accept(*this);
}

void SyntaxVisitor::visit_enum_declaration(const EnumDeclarationSyntax* node)
{
	if (node == nullptr)
		return;

	if (!(node->get_attributes() == nullptr))
		visit_attributes_list(node->get_attributes());

	for (EnumFieldDeclarationSyntax* const& field : node->get_fields())
		field->accept(*this);
}

void SyntaxVisitor::visit_enum_field_declaration(const EnumFieldDeclarationSyntax* node)
{
	if (node == nullptr)
		return;

	if (!(node->get_expression() == nullptr))
		node->get_expression()->accept(*this);
}

void SyntaxVisitor::visit_field_declaration(const FieldDeclarationSyntax* node)
{
	if (node == nullptr)
		return;

	if (!(node->get_attributes() == nullptr))
		visit_attributes_list(node->get_attributes());

	if (!(node->get_type() == nullptr))
		node->get_type()->accept(*this);

	if (!(node->get_expression() == nullptr))
		node->get_expression()->accept(*this);
}

void SyntaxVisitor::visit_property_declaration(const PropertyDeclarationSyntax* node)
{
	if (node == nullptr)
		return;

	if (!(node->get_attributes() == nullptr))
		visit_attributes_list(node->get_attributes());

	if (!(node->get_type() == nullptr))
		node->get_type()->accept(*this);

	if (!(node->get_getter() == nullptr))
		node->get_getter()->accept(*this);

	if (!(node->get_setter() == nullptr))
		node->get_setter()->accept(*this);

	if (!(node->get_expression() == nullptr))
		node->get_expression()->accept(*this);
}

void SyntaxVisitor::visit_indexator_declaration(const IndexatorDeclarationSyntax* node)
{
	if (node == nullptr)
		return;

	if (!(node->get_attributes() == nullptr))
		visit_attributes_list(node->get_attributes());

	if (!(node->get_parameters_list() == nullptr))
		visit_parameters_list(node->get_parameters_list());

	if (!(node->get_type() == nullptr))
		node->get_type()->accept(*this);

	if (!(node->get_getter() == nullptr))
		node->get_getter()->accept(*this);

	if (!(node->get_setter() == nullptr))
		node->get_setter()->accept(*this);
}

void SyntaxVisitor::visit_accessor_declaration(const AccessorDeclarationSyntax* node)
{
	if (node == nullptr)
		return;

	if (!(node->get_attributes() == nullptr))
		visit_attributes_list(node->get_attributes());
}

void SyntaxVisitor::visit_constructor_declaration(const ConstructorDeclarationSyntax* node)
{
	if (node == nullptr)
		return;

	if (!(node->get_attributes() == nullptr))
		visit_attributes_list(node->get_attributes());

	if (!(node->get_parameters_list() == nullptr))
		visit_parameters_list(node->get_parameters_list());

	if (!(node->get_body() == nullptr))
		node->get_body()->accept(*this);
}

void SyntaxVisitor::visit_operator_declaration(const OperatorDeclarationSyntax* node)
{
	if (node == nullptr)
		return;

	if (!(node->get_attributes() == nullptr))
		visit_attributes_list(node->get_attributes());

	if (!(node->get_parameters_list() == nullptr))
		visit_parameters_list(node->get_parameters_list());

	if (!(node->get_return_type() == nullptr))
		node->get_return_type()->accept(*this);

	if (!(node->get_body() == nullptr))
		node->get_body()->accept(*this);
}

void SyntaxVisitor::visit_delegate_declaration(const DelegateDeclarationSyntax* node)
{
	if (node == nullptr)
		return;

	if (!(node->get_attributes() == nullptr))
		visit_attributes_list(node->get_attributes());

	if (!(node->get_type_parameters() == nullptr))
		visit_type_parameters_list(node->get_type_parameters());

	if (!(node->get_where_clauses() == nullptr))
		visit_where_clauses_list(node->get_where_clauses());

	if (!(node->get_parameters_list() == nullptr))
		visit_parameters_list(node->get_parameters_list());

	if (!(node->get_return_type() == nullptr))
		node->get_return_type()->accept(*this);
}

void SyntaxVisitor::visit_function_declaration(const FunctionDeclarationSyntax* node)
{
	if (node == nullptr)
		return;

	if (!(node->get_attributes() == nullptr))
		visit_attributes_list(node->get_attributes());

	if (!(node->get_type_parameters() == nullptr))
		visit_type_parameters_list(node->get_type_parameters());

	if (!(node->get_where_clauses() == nullptr))
		visit_where_clauses_list(node->get_where_clauses());

	if (!(node->get_parameters_list() == nullptr))
		visit_parameters_list(node->get_parameters_list());

	if (!(node->get_return_type() == nullptr))
		node->get_return_type()->accept(*this);

	if (!(node->get_body() == nullptr))
		node->get_body()->accept(*this);
}

void SyntaxVisitor::visit_statements_block(const StatementsBlockSyntax* node)
{
	if (node == nullptr)
		return;

	for (StatementSyntax* const& statement : node->get_statements())
		statement->accept(*this);
}

void SyntaxVisitor::visit_arrow_clause(const ArrowClauseSyntax* node)
{
	if (node == nullptr)
		return;

	if (!(node->get_expression() == nullptr))
		node->get_expression()->accept(*this);
}

void SyntaxVisitor::visit_expression_statement(const ExpressionStatementSyntax* node)
{
	if (node == nullptr)
		return;

	if (!(node->get_expression() == nullptr))
		node->get_expression()->accept(*this);
}

void SyntaxVisitor::visit_defer_statement(const DeferStatementSyntax* node)
{
	if (node == nullptr)
		return;

	if (!(node->get_expression() == nullptr))
		node->get_expression()->accept(*this);
}

void SyntaxVisitor::visit_defer_block(const DeferBlockSyntax* node)
{
	if (node == nullptr)
		return;

	if (!(node->get_block() == nullptr))
		node->get_block()->accept(*this);
}

void SyntaxVisitor::visit_defer_variable(const DeferVariableSyntax* node)
{
	if (node == nullptr)
		return;

	if (!(node->get_variable() == nullptr))
		visit_variable_statement(node->get_variable());
}

void SyntaxVisitor::visit_return_statement(const ReturnStatementSyntax* node)
{
	if (node == nullptr)
		return;

	if (!(node->get_expression() == nullptr))
		node->get_expression()->accept(*this);
}

void SyntaxVisitor::visit_break_statement(const BreakStatementSyntax* node)
{
	if (node == nullptr)
		return;
}

void SyntaxVisitor::visit_continue_statement(const ContinueStatementSyntax* node)
{
	if (node == nullptr)
		return;
}

void SyntaxVisitor::visit_while_statement(const WhileStatementSyntax* node)
{
	if (node == nullptr)
		return;

	if (!(node->get_condition() == nullptr))
		node->get_condition()->accept(*this);

	if (!(node->get_block() == nullptr))
		node->get_block()->accept(*this);
}

void SyntaxVisitor::visit_until_statement(const UntilStatementSyntax* node)
{
	if (node == nullptr)
		return;

	if (!(node->get_condition() == nullptr))
		node->get_condition()->accept(*this);

	if (!(node->get_block() == nullptr))
		node->get_block()->accept(*this);
}

void SyntaxVisitor::visit_for_in_statement(const ForInStatementSyntax* node)
{
	if (node == nullptr)
		return;

	if (!(node->get_range_expression() == nullptr))
		node->get_range_expression()->accept(*this);

	if (!(node->get_block() == nullptr))
		node->get_block()->accept(*this);
}

void SyntaxVisitor::visit_if_statement(const IfStatementSyntax* node)
{
	if (node == nullptr)
		return;

	if (!(node->get_condition() == nullptr))
		node->get_condition()->accept(*this);

	if (!(node->get_block() == nullptr))
		node->get_block()->accept(*this);

	if (!(node->get_next() == nullptr))
		node->get_next()->accept(*this);
}

void SyntaxVisitor::visit_unless_statement(const UnlessStatementSyntax* node)
{
	if (node == nullptr)
		return;

	if (!(node->get_condition() == nullptr))
		node->get_condition()->accept(*this);

	if (!(node->get_block() == nullptr))
		node->get_block()->accept(*this);

	if (!(node->get_next() == nullptr))
		node->get_next()->accept(*this);
}

void SyntaxVisitor::visit_else_statement(const ElseStatementSyntax* node)
{
	if (node == nullptr)
		return;

	if (!(node->get_block() == nullptr))
		node->get_block()->accept(*this);

	if (!(node->get_next() == nullptr))
		node->get_next()->accept(*this);
}

void SyntaxVisitor::visit_throw_statement(const ThrowStatementSyntax* node)
{
	if (node == nullptr)
		return;

	if (!(node->get_expression() == nullptr))
		node->get_expression()->accept(*this);
}

void SyntaxVisitor::visit_try_statement(const TryStatementSyntax* node)
{
	if (node == nullptr)
		return;

	if (!(node->get_try_block() == nullptr))
		node->get_try_block()->accept(*this);

	for (CatchClauseSyntax* const& clause : node->get_catch_clauses())
	{
		if (!(clause == nullptr))
			clause->accept(*this);
	}
}

void SyntaxVisitor::visit_catch_clause(const CatchClauseSyntax* node)
{
	if (node == nullptr)
		return;

	if (!(node->get_exception_type() == nullptr))
		node->get_exception_type()->accept(*this);

	if (!(node->get_body() == nullptr))
		node->get_body()->accept(*this);
}

void SyntaxVisitor::visit_variable_statement(const VariableStatementSyntax* node)
{
	if (node == nullptr)
		return;

	if (!(node->get_type() == nullptr))
		node->get_type()->accept(*this);

	if (!(node->get_expression() == nullptr))
		node->get_expression()->accept(*this);
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

	if (!(node->get_left() == nullptr))
		node->get_left()->accept(*this);

	if (!(node->get_right() == nullptr))
		node->get_right()->accept(*this);
}

void SyntaxVisitor::visit_unary_expression(const UnaryExpressionSyntax* node)
{
	if (node == nullptr)
		return;

	if (!(node->get_operand() == nullptr))
		node->get_operand()->accept(*this);
}

void SyntaxVisitor::visit_member_access_expression(const MemberAccessExpressionSyntax* node)
{
	if (node == nullptr)
		return;

	if (!(node->get_previous() == nullptr))
		node->get_previous()->accept(*this);
}

void SyntaxVisitor::visit_invokation_expression(const InvokationExpressionSyntax* node)
{
	if (node == nullptr)
		return;

	if (!(node->get_previous() == nullptr))
		node->get_previous()->accept(*this);

	if (!(node->get_arguments() == nullptr))
		visit_arguments_list(node->get_arguments());
}

void SyntaxVisitor::visit_indexator_expression(const IndexatorExpressionSyntax* node)
{
	if (node == nullptr)
		return;

	if (!(node->get_previous() == nullptr))
		node->get_previous()->accept(*this);

	if (!(node->get_arguments() == nullptr))
		visit_arguments_list(node->get_arguments());
}

void SyntaxVisitor::visit_is_expression(const IsExpressionSyntax* node)
{
	if (node == nullptr)
		return;

	if (!(node->get_expression() == nullptr))
		node->get_expression()->accept(*this);

	if (!(node->get_target_type() == nullptr))
		node->get_target_type()->accept(*this);
}

void SyntaxVisitor::visit_cast_expression(const CastExpressionSyntax* node)
{
	if (node == nullptr)
		return;

	if (!(node->get_expression() == nullptr))
		node->get_expression()->accept(*this);

	if (!(node->get_target_type() == nullptr))
		node->get_target_type()->accept(*this);
}

void SyntaxVisitor::visit_if_expression(const IfExpressionSyntax* node)
{
	if (node == nullptr)
		return;

	if (!(node->get_condition() == nullptr))
		node->get_condition()->accept(*this);

	if (!(node->get_then_expression() == nullptr))
		node->get_then_expression()->accept(*this);

	if (!(node->get_else_expression() == nullptr))
		node->get_else_expression()->accept(*this);
}

void SyntaxVisitor::visit_await_expression(const AwaitExpressionSyntax* node)
{
	if (node == nullptr)
		return;

	if (!(node->get_expression() == nullptr))
		node->get_expression()->accept(*this);
}

void SyntaxVisitor::visit_range_expression(const RangeExpressionSyntax* node)
{
	if (node == nullptr)
		return;

	if (!(node->get_left() == nullptr))
		node->get_left()->accept(*this);

	if (!(node->get_right() == nullptr))
		node->get_right()->accept(*this);
}

void SyntaxVisitor::visit_collection_expression(const CollectionExpressionSyntax* node)
{
	if (node == nullptr)
		return;

	for (ExpressionSyntax* const& value : node->get_values())
		value->accept(*this);
}

void SyntaxVisitor::visit_object_creation_expression(const ObjectExpressionSyntax* node)
{
	if (node == nullptr)
		return;

	if (!(node->get_type() == nullptr))
		node->get_type()->accept(*this);

	if (!(node->get_arguments() == nullptr))
		visit_arguments_list(node->get_arguments());

	if (!(node->get_array_size() == nullptr))
		node->get_array_size()->accept(*this);
}

void SyntaxVisitor::visit_lambda_expression(const LambdaExpressionSyntax* node)
{
	if (node == nullptr)
		return;

	if (!(node->get_parameters() == nullptr))
		visit_parameters_list(node->get_parameters());

	if (!(node->get_body() == nullptr))
		node->get_body()->accept(*this);
}

void SyntaxVisitor::visit_parameter(const ParameterSyntax* node)
{
	if (node == nullptr)
		return;

	if (!(node->get_type() == nullptr))
		node->get_type()->accept(*this);
}

void SyntaxVisitor::visit_parameters_list(const ParametersListSyntax* node)
{
	if (node == nullptr)
		return;

	for (ParameterSyntax* const& parameter : node->get_parameters())
		visit_parameter(parameter);
}

void SyntaxVisitor::visit_argument(const ArgumentSyntax* node)
{
	if (node == nullptr)
		return;

	if (!(node->get_expression() == nullptr))
		node->get_expression()->accept(*this);
}

void SyntaxVisitor::visit_arguments_list(const ArgumentsListSyntax* node)
{
	if (node == nullptr)
		return;

	for (ArgumentSyntax* const& argument : node->get_arguments())
		visit_argument(argument);
}

void SyntaxVisitor::visit_type_arguments_list(const TypeArgumentsListSyntax* node)
{
	if (node == nullptr)
		return;

	for (TypeSyntax* const& type : node->get_types())
		type->accept(*this);
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

	for (TypeParameterSyntax* const& parameter : node->get_parameters())
		visit_type_parameter(parameter);
}

void SyntaxVisitor::visit_base_types_list(const BaseTypesListSyntax* node)
{
	if (node == nullptr)
		return;

	for (TypeSyntax* const& type : node->get_types())
		type->accept(*this);
}

void SyntaxVisitor::visit_where_clause(const WhereClauseSyntax* node)
{
	if (node == nullptr)
		return;

	for (TypeSyntax* const& constraint : node->get_constraint_types())
		constraint->accept(*this);
}

void SyntaxVisitor::visit_where_clauses_list(const WhereClausesListSyntax* node)
{
	if (node == nullptr)
		return;

	for (WhereClauseSyntax* const& clause : node->get_clauses())
		visit_where_clause(clause);
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

	if (!(node->get_left() == nullptr))
		node->get_left()->accept(*this);
}

void SyntaxVisitor::visit_generic_type(const GenericTypeSyntax* node)
{
	if (node == nullptr)
		return;

	if (!(node->get_underlaying_type() == nullptr))
		node->get_underlaying_type()->accept(*this);

	if (!(node->get_type_arguments() == nullptr))
		visit_type_arguments_list(node->get_type_arguments());
}

void SyntaxVisitor::visit_array_type(const ArrayTypeSyntax* node)
{
	if (node == nullptr)
		return;

	if (!(node->get_underlaying_type() == nullptr))
		node->get_underlaying_type()->accept(*this);
}

void SyntaxVisitor::visit_nullable_type(const NullableTypeSyntax* node)
{
	if (node == nullptr)
		return;

	if (!(node->get_underlaying_type() == nullptr))
		node->get_underlaying_type()->accept(*this);
}