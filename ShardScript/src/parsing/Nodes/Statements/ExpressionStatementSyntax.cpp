#include <shard/parsing/nodes/statements/ExpressionStatementSyntax.hpp>
#include <shard/parsing/SyntaxVisitor.hpp>

#include <gmt/Arena.hpp>

using namespace shard;

ExpressionStatementSyntax::ExpressionStatementSyntax(SyntaxNode* parent)
	: StatementSyntax(SyntaxKind::ExpressionStatement, parent) { }

const ExpressionSyntax* ExpressionStatementSyntax::get_expression() const
{
	return m_expression;
}

SyntaxToken ExpressionStatementSyntax::get_semicolon() const
{
	return m_semicolonToken;
}

void ExpressionStatementSyntax::set_expression(ExpressionSyntax* expression)
{
	m_expression = expression;
}

void ExpressionStatementSyntax::set_semicolon(const SyntaxToken& token)
{
	m_semicolonToken = token;
}

TextLocation ExpressionStatementSyntax::get_location() const
{
	if (!(m_expression == nullptr))
		return TextLocation(m_expression->get_location(), m_semicolonToken);

	return m_semicolonToken.get_location();
}

void ExpressionStatementSyntax::accept(SyntaxVisitor& visitor) const
{
	visitor.visit_expression_statement(this);
}