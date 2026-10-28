#include <shard/parsing/nodes/statements/ThrowStatementSyntax.hpp>
#include <shard/parsing/SyntaxVisitor.hpp>

#include <gmt/Arena.hpp>

using namespace shard;

ThrowStatementSyntax::ThrowStatementSyntax(SyntaxNode* parent)
	: StatementSyntax(SyntaxKind::ThrowStatement, parent) { }

SyntaxToken ThrowStatementSyntax::get_throw_keyword() const
{
	return m_throwKeywordToken;
}

const ExpressionSyntax* ThrowStatementSyntax::get_expression() const
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

void ThrowStatementSyntax::set_expression(ExpressionSyntax* expression)
{
	m_expression = expression;
}

void ThrowStatementSyntax::set_semicolon(const SyntaxToken& token)
{
	m_semicolonToken = token;
}

TextLocation ThrowStatementSyntax::get_location() const
{
	if (!(m_expression == nullptr))
		return TextLocation(m_throwKeywordToken, m_expression->get_location());

	return TextLocation(m_throwKeywordToken, m_semicolonToken);
}

void ThrowStatementSyntax::accept(SyntaxVisitor& visitor) const
{
	visitor.visit_throw_statement(this);
}