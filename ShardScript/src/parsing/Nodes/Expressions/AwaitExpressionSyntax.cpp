#include <shard/parsing/nodes/expressions/AwaitExpressionSyntax.hpp>
#include <shard/parsing/SyntaxVisitor.hpp>

#include <gmt/Arena.hpp>

using namespace shard;

AwaitExpressionSyntax::AwaitExpressionSyntax(gmt::Ref<SyntaxNode> parent)
	: ExpressionSyntax(SyntaxKind::AwaitExpression, parent) { }

SyntaxToken AwaitExpressionSyntax::get_await_keyword() const
{
	return m_awaitKeywordToken;
}

gmt::Ref<const ExpressionSyntax> AwaitExpressionSyntax::get_expression() const
{
	return m_expression;
}

void AwaitExpressionSyntax::set_await_keyword(const SyntaxToken& token)
{
	m_awaitKeywordToken = token;
}

void AwaitExpressionSyntax::set_expression(gmt::Ref<ExpressionSyntax> expression)
{
	m_expression = expression;
}

TextLocation AwaitExpressionSyntax::get_location() const
{
	if (!m_expression.is_null())
		return TextLocation(m_awaitKeywordToken, m_expression.as_ptr()->get_location());

	return m_awaitKeywordToken.get_location();
}

void AwaitExpressionSyntax::accept(SyntaxVisitor& visitor) const
{
	visitor.visit_await_expression(this);
}
