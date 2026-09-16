#include <shard/parsing/nodes/expressions/IsExpressionSyntax.hpp>
#include <shard/parsing/SyntaxVisitor.hpp>

#include <gmt/Arena.hpp>

using namespace shard;

IsExpressionSyntax::IsExpressionSyntax(gmt::Ref<SyntaxNode> parent)
	: ExpressionSyntax(SyntaxKind::IsExpression, parent) { }

SyntaxToken IsExpressionSyntax::get_operator_token() const
{
	return m_operatorToken;
}

gmt::Ref<const ExpressionSyntax> IsExpressionSyntax::get_expression() const
{
	return m_expression;
}

gmt::Ref<const TypeSyntax> IsExpressionSyntax::get_target_type() const
{
	return m_targetType;
}

void IsExpressionSyntax::set_operator_token(const SyntaxToken& token)
{
	m_operatorToken = token;
}

void IsExpressionSyntax::set_expression(gmt::Ref<ExpressionSyntax> expression)
{
	m_expression = expression;
}

void IsExpressionSyntax::set_target_type(gmt::Ref<TypeSyntax> targetType)
{
	m_targetType = targetType;
}

TextLocation IsExpressionSyntax::get_location() const
{
	if (!m_expression.is_null())
	{
		if (!m_targetType.is_null())
			return TextLocation(m_expression.as_ptr()->get_location(), m_targetType.as_ptr()->get_location());

		return m_expression.as_ptr()->get_location();
	}

	return m_operatorToken.get_location();
}

void IsExpressionSyntax::accept(SyntaxVisitor& visitor) const
{
	visitor.visit_is_expression(this);
}
