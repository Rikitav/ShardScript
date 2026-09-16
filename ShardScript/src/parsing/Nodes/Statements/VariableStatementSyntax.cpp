#include <shard/parsing/nodes/statements/VariableStatementSyntax.hpp>
#include <shard/parsing/SyntaxVisitor.hpp>

#include <gmt/Arena.hpp>

using namespace shard;

VariableStatementSyntax::VariableStatementSyntax(gmt::Ref<SyntaxNode> parent)
	: StatementSyntax(SyntaxKind::VariableStatement, parent) { }

SyntaxToken VariableStatementSyntax::get_identifier() const
{
	return m_identifierToken;
}

SyntaxToken VariableStatementSyntax::get_assign_token() const
{
	return m_assignToken;
}

SyntaxToken VariableStatementSyntax::get_semicolon() const
{
	return m_semicolonToken;
}

gmt::Ref<const TypeSyntax> VariableStatementSyntax::get_type() const
{
	return m_type;
}

gmt::Ref<const ExpressionSyntax> VariableStatementSyntax::get_expression() const
{
	return m_expression;
}

void VariableStatementSyntax::set_identifier(const SyntaxToken& token)
{
	m_identifierToken = token;
}

void VariableStatementSyntax::set_assign_token(const SyntaxToken& token)
{
	m_assignToken = token;
}

void VariableStatementSyntax::set_semicolon(const SyntaxToken& token)
{
	m_semicolonToken = token;
}

void VariableStatementSyntax::set_type(gmt::Ref<TypeSyntax> type)
{
	m_type = type;
}

void VariableStatementSyntax::set_expression(gmt::Ref<ExpressionSyntax> expression)
{
	m_expression = expression;
}

TextLocation VariableStatementSyntax::get_location() const
{
	return TextLocation(m_identifierToken, m_semicolonToken);
}

void VariableStatementSyntax::accept(SyntaxVisitor& visitor) const
{
	visitor.visit_variable_statement(this);
}
