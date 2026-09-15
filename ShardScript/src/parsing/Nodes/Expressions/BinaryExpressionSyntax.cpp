#include <shard/parsing/nodes/expressions/BinaryExpressionSyntax.hpp>
#include <shard/parsing/SyntaxVisitor.hpp>

#include <gmt/Arena.hpp>

using namespace shard;

BinaryExpressionSyntax::BinaryExpressionSyntax(gmt::Ref<SyntaxNode> parent)
	: ExpressionSyntax(SyntaxKind::BinaryExpression, parent) { }

SyntaxToken BinaryExpressionSyntax::get_operator_token() const
{
	return m_operatorToken;
}

gmt::Ref<const ExpressionSyntax> BinaryExpressionSyntax::get_left() const
{
	return m_left;
}

gmt::Ref<const ExpressionSyntax> BinaryExpressionSyntax::get_right() const
{
	return m_right;
}

void BinaryExpressionSyntax::set_operator_token(const SyntaxToken& token)
{
	m_operatorToken = token;
}

void BinaryExpressionSyntax::set_left(gmt::Ref<ExpressionSyntax> left)
{
	m_left = left;
}

void BinaryExpressionSyntax::set_right(gmt::Ref<ExpressionSyntax> right)
{
	m_right = right;
}

TextLocation BinaryExpressionSyntax::get_location() const
{
	if (!m_left.is_null())
		return m_left.get()->get_location();

	return m_operatorToken.get_location();
}

void BinaryExpressionSyntax::accept(SyntaxVisitor& visitor) const
{
	visitor.visit_binary_expression(this);
}
