#include <shard/parsing/nodes/expressions/IsExpressionSyntax.hpp>
#include <shard/parsing/SyntaxVisitor.hpp>

#include <gmt/Arena.hpp>

using namespace shard;

IsExpressionSyntax::IsExpressionSyntax(SyntaxNode* parent)
	: ExpressionSyntax(SyntaxKind::IsExpression, parent) { }

SyntaxToken IsExpressionSyntax::get_operator_token() const
{
	return m_operatorToken;
}

const ExpressionSyntax* IsExpressionSyntax::get_expression() const
{
	return m_expression;
}

const TypeSyntax* IsExpressionSyntax::get_target_type() const
{
	return m_targetType;
}

void IsExpressionSyntax::set_operator_token(const SyntaxToken& token)
{
	m_operatorToken = token;
}

void IsExpressionSyntax::set_expression(ExpressionSyntax* expression)
{
	m_expression = expression;
}

void IsExpressionSyntax::set_target_type(TypeSyntax* targetType)
{
	m_targetType = targetType;
}

TextLocation IsExpressionSyntax::get_location() const
{
	if (!(m_expression == nullptr))
	{
		if (!(m_targetType == nullptr))
			return TextLocation(m_expression->get_location(), m_targetType->get_location());

		return m_expression->get_location();
	}

	return m_operatorToken.get_location();
}

void IsExpressionSyntax::accept(SyntaxVisitor& visitor) const
{
	visitor.visit_is_expression(this);
}