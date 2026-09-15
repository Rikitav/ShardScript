#include <shard/parsing/nodes/expressions/LiteralExpressionsSyntax.hpp>
#include <shard/parsing/SyntaxVisitor.hpp>

#include <gmt/Arena.hpp>

using namespace shard;

LiteralExpressionSyntax::LiteralExpressionSyntax(gmt::Ref<SyntaxNode> parent)
	: ExpressionSyntax(SyntaxKind::LiteralExpression, parent) { }

SyntaxToken LiteralExpressionSyntax::get_literal() const
{
	return m_literalToken;
}

void LiteralExpressionSyntax::set_literal(const SyntaxToken& token)
{
	m_literalToken = token;
}

TextLocation LiteralExpressionSyntax::get_location() const
{
	return m_literalToken.get_location();
}

void LiteralExpressionSyntax::accept(SyntaxVisitor& visitor) const
{
	visitor.visit_literal_expression(this);
}
