#include <shard/parsing/SyntaxVisitor.hpp>
#include <shard/parsing/SyntaxTree.hpp>

using namespace shard;

void SyntaxVisitor::visit_syntax_tree(const SyntaxTree& tree)
{
	for (const auto& unitReference : tree.get_units())
	{
		const TranslationUnitSyntax* unit = unitReference.get();
		visit_translation_unit(unit);

		for (const gmt::Ref<UsingDirectiveSyntax>& directive : unit->get_usings().get())
			visit_using_directive(directive.get());

		if (unit->has_namespace())
			visit_namespace_directive(unit->get_namespace().get());

		for (const gmt::Ref<MemberDeclarationSyntax>& member : unit->get_members().get())
			member.get()->accept(*this);
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

	for (const gmt::Ref<AttributeSyntax>& attribute : node->get_attributes().get())
		visit_attribute(attribute.get());
}

void SyntaxVisitor::visit_class_declaration(const ClassDeclarationSyntax* node)
{
	if (node == nullptr)
		return;

	if (!node->get_attributes().is_null())
		visit_attributes_list(node->get_attributes().get());

	if (!node->get_type_parameters().is_null())
		visit_type_parameters_list(node->get_type_parameters().get());

	if (!node->get_where_clauses().is_null())
		visit_where_clauses_list(node->get_where_clauses().get());

	if (!node->get_base_types().is_null())
		visit_base_types_list(node->get_base_types().get());

	for (const gmt::Ref<MemberDeclarationSyntax>& member : node->get_members().get())
		member.get()->accept(*this);
}

void SyntaxVisitor::visit_function_declaration(const FunctionDeclarationSyntax* node)
{
	if (node == nullptr)
		return;

	if (!node->get_attributes().is_null())
		visit_attributes_list(node->get_attributes().get());

	if (!node->get_type_parameters().is_null())
		visit_type_parameters_list(node->get_type_parameters().get());

	if (!node->get_where_clauses().is_null())
		visit_where_clauses_list(node->get_where_clauses().get());

	if (!node->get_parameters_list().is_null())
		visit_parameters_list(node->get_parameters_list().get());

	if (!node->get_return_type().is_null())
		node->get_return_type().get()->accept(*this);

	if (!node->get_body().is_null())
		node->get_body().get()->accept(*this);
}

void SyntaxVisitor::visit_statements_block(const StatementsBlockSyntax* node)
{
	if (node == nullptr)
		return;

	for (const gmt::Ref<StatementSyntax>& statement : node->get_statements().get())
		statement.get()->accept(*this);
}

void SyntaxVisitor::visit_arrow_clause(const ArrowClauseSyntax* node)
{
	if (node == nullptr)
		return;

	if (!node->get_expression().is_null())
		node->get_expression().get()->accept(*this);
}

void SyntaxVisitor::visit_expression_statement(const ExpressionStatementSyntax* node)
{
	if (node == nullptr)
		return;

	if (!node->get_expression().is_null())
		node->get_expression().get()->accept(*this);
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
		node->get_left().get()->accept(*this);

	if (!node->get_right().is_null())
		node->get_right().get()->accept(*this);
}

void SyntaxVisitor::visit_member_access_expression(const MemberAccessExpressionSyntax* node)
{
	if (node == nullptr)
		return;

	if (!node->get_previous().is_null())
		node->get_previous().get()->accept(*this);
}

void SyntaxVisitor::visit_invokation_expression(const InvokationExpressionSyntax* node)
{
	if (node == nullptr)
		return;

	if (!node->get_previous().is_null())
		node->get_previous().get()->accept(*this);

	if (!node->get_arguments().is_null())
		visit_arguments_list(node->get_arguments().get());
}

void SyntaxVisitor::visit_parameter(const ParameterSyntax* node)
{
	if (node == nullptr)
		return;

	if (!node->get_type().is_null())
		node->get_type().get()->accept(*this);
}

void SyntaxVisitor::visit_parameters_list(const ParametersListSyntax* node)
{
	if (node == nullptr)
		return;

	for (const gmt::Ref<ParameterSyntax>& parameter : node->get_parameters().get())
		visit_parameter(parameter.get());
}

void SyntaxVisitor::visit_argument(const ArgumentSyntax* node)
{
	if (node == nullptr)
		return;

	if (!node->get_expression().is_null())
		node->get_expression().get()->accept(*this);
}

void SyntaxVisitor::visit_arguments_list(const ArgumentsListSyntax* node)
{
	if (node == nullptr)
		return;

	for (const gmt::Ref<ArgumentSyntax>& argument : node->get_arguments().get())
		visit_argument(argument.get());
}

void SyntaxVisitor::visit_type_arguments_list(const TypeArgumentsListSyntax* node)
{
	if (node == nullptr)
		return;

	for (const gmt::Ref<TypeSyntax>& type : node->get_types().get())
		type.get()->accept(*this);
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

	for (const gmt::Ref<TypeParameterSyntax>& parameter : node->get_parameters().get())
		visit_type_parameter(parameter.get());
}

void SyntaxVisitor::visit_base_types_list(const BaseTypesListSyntax* node)
{
	if (node == nullptr)
		return;

	for (const gmt::Ref<TypeSyntax>& type : node->get_types().get())
		type.get()->accept(*this);
}

void SyntaxVisitor::visit_where_clause(const WhereClauseSyntax* node)
{
	if (node == nullptr)
		return;

	for (const gmt::Ref<TypeSyntax>& constraint : node->get_constraint_types().get())
		constraint.get()->accept(*this);
}

void SyntaxVisitor::visit_where_clauses_list(const WhereClausesListSyntax* node)
{
	if (node == nullptr)
		return;

	for (const gmt::Ref<WhereClauseSyntax>& clause : node->get_clauses().get())
		visit_where_clause(clause.get());
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
		node->get_left().get()->accept(*this);
}

void SyntaxVisitor::visit_generic_type(const GenericTypeSyntax* node)
{
	if (node == nullptr)
		return;

	if (!node->get_underlaying_type().is_null())
		node->get_underlaying_type().get()->accept(*this);

	if (!node->get_type_arguments().is_null())
		visit_type_arguments_list(node->get_type_arguments().get());
}

void SyntaxVisitor::visit_array_type(const ArrayTypeSyntax* node)
{
	if (node == nullptr)
		return;

	if (!node->get_underlaying_type().is_null())
		node->get_underlaying_type().get()->accept(*this);
}

void SyntaxVisitor::visit_nullable_type(const NullableTypeSyntax* node)
{
	if (node == nullptr)
		return;

	if (!node->get_underlaying_type().is_null())
		node->get_underlaying_type().get()->accept(*this);
}
