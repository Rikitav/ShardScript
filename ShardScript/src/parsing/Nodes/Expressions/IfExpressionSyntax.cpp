#include <shard/parsing/nodes/expressions/IfExpressionSyntax.hpp>
#include <shard/parsing/SyntaxVisitor.hpp>

#include <gmt/Arena.hpp>

using namespace shard;

IfExpressionSyntax::IfExpressionSyntax(gmt::Ref<SyntaxNode> parent)
	: ExpressionSyntax(SyntaxKind::IfExpression, parent) { }

SyntaxToken IfExpressionSyntax::get_if_keyword() const
{
	return m_ifKeywordToken;
}

gmt::Ref<const ExpressionSyntax> IfExpressionSyntax::get_condition() const
{
	return m_condition;
}

gmt::Ref<const ExpressionSyntax> IfExpressionSyntax::get_then_expression() const
{
	return m_thenExpression;
}

SyntaxToken IfExpressionSyntax::get_else_keyword() const
{
	return m_elseKeywordToken;
}

gmt::Ref<const ExpressionSyntax> IfExpressionSyntax::get_else_expression() const
{
	return m_elseExpression;
}

void IfExpressionSyntax::set_if_keyword(const SyntaxToken& token)
{
	m_ifKeywordToken = token;
}

void IfExpressionSyntax::set_condition(gmt::Ref<ExpressionSyntax> condition)
{
	m_condition = condition;
}

void IfExpressionSyntax::set_then_expression(gmt::Ref<ExpressionSyntax> thenExpression)
{
	m_thenExpression = thenExpression;
}

void IfExpressionSyntax::set_else_keyword(const SyntaxToken& token)
{
	m_elseKeywordToken = token;
}

void IfExpressionSyntax::set_else_expression(gmt::Ref<ExpressionSyntax> elseExpression)
{
	m_elseExpression = elseExpression;
}

TextLocation IfExpressionSyntax::get_location() const
{
	if (!m_elseExpression.is_null())
		return TextLocation(m_ifKeywordToken, m_elseExpression.as_ptr()->get_location());

	if (!m_thenExpression.is_null())
		return TextLocation(m_ifKeywordToken, m_thenExpression.as_ptr()->get_location());

	return m_ifKeywordToken.get_location();
}

void IfExpressionSyntax::accept(SyntaxVisitor& visitor) const
{
	visitor.visit_if_expression(this);
}
