#include <shard/parsing/nodes/statements/DeferStatementSyntax.hpp>
#include <shard/parsing/SyntaxVisitor.hpp>

#include <gmt/Arena.hpp>

using namespace shard;

DeferStatementSyntax::DeferStatementSyntax(gmt::Ref<SyntaxNode> parent)
	: StatementSyntax(SyntaxKind::DeferStatement, parent) { }

SyntaxToken DeferStatementSyntax::get_defer_keyword() const
{
	return m_deferKeywordToken;
}

gmt::Ref<const ExpressionSyntax> DeferStatementSyntax::get_expression() const
{
	return m_expression;
}

SyntaxToken DeferStatementSyntax::get_semicolon() const
{
	return m_semicolonToken;
}

void DeferStatementSyntax::set_defer_keyword(const SyntaxToken& token)
{
	m_deferKeywordToken = token;
}

void DeferStatementSyntax::set_expression(gmt::Ref<ExpressionSyntax> expression)
{
	m_expression = expression;
}

void DeferStatementSyntax::set_semicolon(const SyntaxToken& token)
{
	m_semicolonToken = token;
}

TextLocation DeferStatementSyntax::get_location() const
{
	return TextLocation(m_deferKeywordToken, m_semicolonToken);
}

void DeferStatementSyntax::accept(SyntaxVisitor& visitor) const
{
	visitor.visit_defer_statement(this);
}
