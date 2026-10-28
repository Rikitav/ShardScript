#include <shard/parsing/nodes/expressions/BinaryExpressionSyntax.hpp>
#include <shard/parsing/SyntaxVisitor.hpp>

#include <gmt/Arena.hpp>

using namespace shard;

BinaryExpressionSyntax::BinaryExpressionSyntax(SyntaxNode* parent)
	: ExpressionSyntax(SyntaxKind::BinaryExpression, parent) { }

SyntaxToken BinaryExpressionSyntax::get_operator_token() const
{
	return m_operatorToken;
}

const ExpressionSyntax* BinaryExpressionSyntax::get_left() const
{
	return m_left;
}

const ExpressionSyntax* BinaryExpressionSyntax::get_right() const
{
	return m_right;
}

void BinaryExpressionSyntax::set_operator_token(const SyntaxToken& token)
{
	m_operatorToken = token;
}

void BinaryExpressionSyntax::set_left(ExpressionSyntax* left)
{
	m_left = left;
}

void BinaryExpressionSyntax::set_right(ExpressionSyntax* right)
{
	m_right = right;
}

TextLocation BinaryExpressionSyntax::get_location() const
{
	if (!(m_left == nullptr) && !(m_right == nullptr))
		return TextLocation(m_left->get_location(), m_right->get_location());

	if (!(m_left == nullptr))
		return m_left->get_location();

	return m_operatorToken.get_location();
}

void BinaryExpressionSyntax::accept(SyntaxVisitor& visitor) const
{
	visitor.visit_binary_expression(this);
}