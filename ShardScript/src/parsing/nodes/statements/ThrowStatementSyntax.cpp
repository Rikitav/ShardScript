#include <shard/parsing/nodes/statements/ThrowStatementSyntax.hpp>
#include <shard/parsing/SyntaxVisitor.hpp>

#include <gmt/Arena.hpp>

using namespace shard;

ThrowStatementSyntax::ThrowStatementSyntax(gmt::Ref<SyntaxNode> parent)
	: StatementSyntax(SyntaxKind::ThrowStatement, parent) { }

SyntaxToken ThrowStatementSyntax::get_throw_keyword() const
{
	return m_throwKeywordToken;
}

gmt::Ref<const ExpressionSyntax> ThrowStatementSyntax::get_expression() const
{
	return m_expression;
}

SyntaxToken ThrowStatementSyntax::get_semicolon() const
{
	return m_semicolonToken;
}

void ThrowStatementSyntax::set_throw_keyword(const SyntaxToken& token)
{
	m_throwKeywordToken = token;
}

void ThrowStatementSyntax::set_expression(gmt::Ref<ExpressionSyntax> expression)
{
	m_expression = expression;
}

void ThrowStatementSyntax::set_semicolon(const SyntaxToken& token)
{
	m_semicolonToken = token;
}

TextLocation ThrowStatementSyntax::get_location() const
{
	if (!m_expression.is_null())
		return TextLocation(m_throwKeywordToken, m_expression.as_ptr()->get_location());

	return TextLocation(m_throwKeywordToken, m_semicolonToken);
}

void ThrowStatementSyntax::accept(SyntaxVisitor& visitor) const
{
	visitor.visit_throw_statement(this);
}
