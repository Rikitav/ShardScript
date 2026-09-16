#include <shard/parsing/nodes/expressions/LambdaExpressionSyntax.hpp>
#include <shard/parsing/SyntaxVisitor.hpp>

#include <gmt/Arena.hpp>

using namespace shard;

LambdaExpressionSyntax::LambdaExpressionSyntax(gmt::Ref<SyntaxNode> parent)
	: ExpressionSyntax(SyntaxKind::LambdaExpression, parent) { }

gmt::Ref<const ParametersListSyntax> LambdaExpressionSyntax::get_parameters() const
{
	return m_parameters;
}

gmt::Ref<const BodySyntax> LambdaExpressionSyntax::get_body() const
{
	return m_body;
}

void LambdaExpressionSyntax::set_parameters(gmt::Ref<ParametersListSyntax> parameters)
{
	m_parameters = parameters;
}

void LambdaExpressionSyntax::set_body(gmt::Ref<BodySyntax> body)
{
	m_body = body;
}

TextLocation LambdaExpressionSyntax::get_location() const
{
	if (!m_body.is_null())
		return TextLocation(m_parameters.as_ptr()->get_open_token(), m_body.as_ptr()->get_location());

	return m_parameters.as_ptr()->get_location();
}

void LambdaExpressionSyntax::accept(SyntaxVisitor& visitor) const
{
	visitor.visit_lambda_expression(this);
}
