#include <shard/parsing/nodes/expressions/RangeExpressionSyntax.hpp>
#include <shard/parsing/SyntaxVisitor.hpp>

#include <gmt/Arena.hpp>

using namespace shard;

RangeExpressionSyntax::RangeExpressionSyntax(gmt::Ref<SyntaxNode> parent)
	: ExpressionSyntax(SyntaxKind::RangeExpression, parent) { }

SyntaxToken RangeExpressionSyntax::get_operator_token() const
{
	return m_operatorToken;
}

gmt::Ref<const ExpressionSyntax> RangeExpressionSyntax::get_left() const
{
	return m_left;
}

gmt::Ref<const ExpressionSyntax> RangeExpressionSyntax::get_right() const
{
	return m_right;
}

bool RangeExpressionSyntax::is_inclusive() const
{
	return m_operatorToken.get_type() == TokenType::RangeInclusiveOperator;
}

void RangeExpressionSyntax::set_operator_token(const SyntaxToken& token)
{
	m_operatorToken = token;
}

void RangeExpressionSyntax::set_left(gmt::Ref<ExpressionSyntax> left)
{
	m_left = left;
}

void RangeExpressionSyntax::set_right(gmt::Ref<ExpressionSyntax> right)
{
	m_right = right;
}

TextLocation RangeExpressionSyntax::get_location() const
{
	if (!m_left.is_null() && !m_right.is_null())
		return TextLocation(m_left.as_ptr()->get_location(), m_right.as_ptr()->get_location());

	if (!m_left.is_null())
		return m_left.as_ptr()->get_location();

	return m_operatorToken.get_location();
}

void RangeExpressionSyntax::accept(SyntaxVisitor& visitor) const
{
	visitor.visit_range_expression(this);
}
