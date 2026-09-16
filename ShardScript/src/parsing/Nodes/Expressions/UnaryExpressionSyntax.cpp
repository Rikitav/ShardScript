#include <shard/parsing/nodes/expressions/UnaryExpressionSyntax.hpp>
#include <shard/parsing/SyntaxVisitor.hpp>

#include <gmt/Arena.hpp>

using namespace shard;

UnaryExpressionSyntax::UnaryExpressionSyntax(gmt::Ref<SyntaxNode> parent)
	: ExpressionSyntax(SyntaxKind::UnaryExpression, parent),
	m_isPostfix(false)
{ }

SyntaxToken UnaryExpressionSyntax::get_operator_token() const
{
	return m_operatorToken;
}

gmt::Ref<const ExpressionSyntax> UnaryExpressionSyntax::get_operand() const
{
	return m_operand;
}

bool UnaryExpressionSyntax::get_is_postfix() const
{
	return m_isPostfix;
}

void UnaryExpressionSyntax::set_operator_token(const SyntaxToken& token)
{
	m_operatorToken = token;
}

void UnaryExpressionSyntax::set_operand(gmt::Ref<ExpressionSyntax> operand)
{
	m_operand = operand;
}

void UnaryExpressionSyntax::set_is_postfix(bool isPostfix)
{
	m_isPostfix = isPostfix;
}

TextLocation UnaryExpressionSyntax::get_location() const
{
	if (!m_operand.is_null())
		return TextLocation(m_operatorToken, m_operand.as_ptr()->get_location());

	return m_operatorToken.get_location();
}

void UnaryExpressionSyntax::accept(SyntaxVisitor& visitor) const
{
	visitor.visit_unary_expression(this);
}
