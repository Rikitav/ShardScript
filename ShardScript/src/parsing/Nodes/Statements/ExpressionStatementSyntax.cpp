#include <shard/parsing/nodes/statements/ExpressionStatementSyntax.hpp>
#include <shard/parsing/SyntaxVisitor.hpp>

#include <gmt/Arena.hpp>

using namespace shard;

ExpressionStatementSyntax::ExpressionStatementSyntax(gmt::Ref<SyntaxNode> parent)
	: StatementSyntax(SyntaxKind::ExpressionStatement, parent) { }

gmt::Ref<const ExpressionSyntax> ExpressionStatementSyntax::get_expression() const
{
	return m_expression;
}

SyntaxToken ExpressionStatementSyntax::get_semicolon() const
{
	return m_semicolonToken;
}

void ExpressionStatementSyntax::set_expression(gmt::Ref<ExpressionSyntax> expression)
{
	m_expression = expression;
}

void ExpressionStatementSyntax::set_semicolon(const SyntaxToken& token)
{
	m_semicolonToken = token;
}

TextLocation ExpressionStatementSyntax::get_location() const
{
	if (!m_expression.is_null())
		return TextLocation(m_expression.as_ptr()->get_location(), m_semicolonToken);

	return m_semicolonToken.get_location();
}

void ExpressionStatementSyntax::accept(SyntaxVisitor& visitor) const
{
	visitor.visit_expression_statement(this);
}
