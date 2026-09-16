#include <shard/parsing/nodes/statements/ReturnStatementSyntax.hpp>
#include <shard/parsing/SyntaxVisitor.hpp>

#include <gmt/Arena.hpp>

using namespace shard;

ReturnStatementSyntax::ReturnStatementSyntax(gmt::Ref<SyntaxNode> parent)
	: StatementSyntax(SyntaxKind::ReturnStatement, parent) { }

SyntaxToken ReturnStatementSyntax::get_return_keyword() const
{
	return m_returnKeywordToken;
}

gmt::Ref<const ExpressionSyntax> ReturnStatementSyntax::get_expression() const
{
	return m_expression;
}

SyntaxToken ReturnStatementSyntax::get_semicolon() const
{
	return m_semicolonToken;
}

void ReturnStatementSyntax::set_return_keyword(const SyntaxToken& token)
{
	m_returnKeywordToken = token;
}

void ReturnStatementSyntax::set_expression(gmt::Ref<ExpressionSyntax> expression)
{
	m_expression = expression;
}

void ReturnStatementSyntax::set_semicolon(const SyntaxToken& token)
{
	m_semicolonToken = token;
}

TextLocation ReturnStatementSyntax::get_location() const
{
	if (!m_expression.is_null())
		return TextLocation(m_returnKeywordToken, m_expression.as_ptr()->get_location());

	return TextLocation(m_returnKeywordToken, m_semicolonToken);
}

void ReturnStatementSyntax::accept(SyntaxVisitor& visitor) const
{
	visitor.visit_return_statement(this);
}
