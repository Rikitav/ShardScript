#include <shard/parsing/nodes/expressions/CastExpressionSyntax.hpp>
#include <shard/parsing/SyntaxVisitor.hpp>

#include <gmt/Arena.hpp>

using namespace shard;

CastExpressionSyntax::CastExpressionSyntax(gmt::Ref<SyntaxNode> parent)
	: ExpressionSyntax(SyntaxKind::CastExpression, parent) { }

SyntaxToken CastExpressionSyntax::get_operator_token() const
{
	return m_operatorToken;
}

gmt::Ref<const ExpressionSyntax> CastExpressionSyntax::get_expression() const
{
	return m_expression;
}

gmt::Ref<const TypeSyntax> CastExpressionSyntax::get_target_type() const
{
	return m_targetType;
}

void CastExpressionSyntax::set_operator_token(const SyntaxToken& token)
{
	m_operatorToken = token;
}

void CastExpressionSyntax::set_expression(gmt::Ref<ExpressionSyntax> expression)
{
	m_expression = expression;
}

void CastExpressionSyntax::set_target_type(gmt::Ref<TypeSyntax> targetType)
{
	m_targetType = targetType;
}

TextLocation CastExpressionSyntax::get_location() const
{
	if (!m_expression.is_null())
	{
		if (!m_targetType.is_null())
			return TextLocation(m_expression.as_ptr()->get_location(), m_targetType.as_ptr()->get_location());

		return m_expression.as_ptr()->get_location();
	}

	return m_operatorToken.get_location();
}

void CastExpressionSyntax::accept(SyntaxVisitor& visitor) const
{
	visitor.visit_cast_expression(this);
}
