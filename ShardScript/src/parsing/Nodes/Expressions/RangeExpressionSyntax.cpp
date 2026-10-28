#include <shard/parsing/nodes/expressions/RangeExpressionSyntax.hpp>
#include <shard/parsing/SyntaxVisitor.hpp>

#include <gmt/Arena.hpp>

using namespace shard;

RangeExpressionSyntax::RangeExpressionSyntax(SyntaxNode* parent)
	: ExpressionSyntax(SyntaxKind::RangeExpression, parent) { }

SyntaxToken RangeExpressionSyntax::get_operator_token() const
{
	return m_operatorToken;
}

const ExpressionSyntax* RangeExpressionSyntax::get_left() const
{
	return m_left;
}

const ExpressionSyntax* RangeExpressionSyntax::get_right() const
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

void RangeExpressionSyntax::set_left(ExpressionSyntax* left)
{
	m_left = left;
}

void RangeExpressionSyntax::set_right(ExpressionSyntax* right)
{
	m_right = right;
}

TextLocation RangeExpressionSyntax::get_location() const
{
	if (!(m_left == nullptr) && !(m_right == nullptr))
		return TextLocation(m_left->get_location(), m_right->get_location());

	if (!(m_left == nullptr))
		return m_left->get_location();

	return m_operatorToken.get_location();
}

void RangeExpressionSyntax::accept(SyntaxVisitor& visitor) const
{
	visitor.visit_range_expression(this);
}