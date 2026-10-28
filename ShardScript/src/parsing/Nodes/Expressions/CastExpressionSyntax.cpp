#include <shard/parsing/nodes/expressions/CastExpressionSyntax.hpp>
#include <shard/parsing/SyntaxVisitor.hpp>

#include <gmt/Arena.hpp>

using namespace shard;

CastExpressionSyntax::CastExpressionSyntax(SyntaxNode* parent)
	: ExpressionSyntax(SyntaxKind::CastExpression, parent) { }

SyntaxToken CastExpressionSyntax::get_operator_token() const
{
	return m_operatorToken;
}

const ExpressionSyntax* CastExpressionSyntax::get_expression() const
{
	return m_expression;
}

const TypeSyntax* CastExpressionSyntax::get_target_type() const
{
	return m_targetType;
}

void CastExpressionSyntax::set_operator_token(const SyntaxToken& token)
{
	m_operatorToken = token;
}

void CastExpressionSyntax::set_expression(ExpressionSyntax* expression)
{
	m_expression = expression;
}

void CastExpressionSyntax::set_target_type(TypeSyntax* targetType)
{
	m_targetType = targetType;
}

TextLocation CastExpressionSyntax::get_location() const
{
	if (!(m_expression == nullptr))
	{
		if (!(m_targetType == nullptr))
			return TextLocation(m_expression->get_location(), m_targetType->get_location());

		return m_expression->get_location();
	}

	return m_operatorToken.get_location();
}

void CastExpressionSyntax::accept(SyntaxVisitor& visitor) const
{
	visitor.visit_cast_expression(this);
}