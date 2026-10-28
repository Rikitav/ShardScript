#include <shard/parsing/nodes/expressions/LambdaExpressionSyntax.hpp>
#include <shard/parsing/SyntaxVisitor.hpp>

#include <gmt/Arena.hpp>

using namespace shard;

LambdaExpressionSyntax::LambdaExpressionSyntax(SyntaxNode* parent)
	: ExpressionSyntax(SyntaxKind::LambdaExpression, parent) { }

const ParametersListSyntax* LambdaExpressionSyntax::get_parameters() const
{
	return m_parameters;
}

const BlockSyntax* LambdaExpressionSyntax::get_body() const
{
	return m_body;
}

void LambdaExpressionSyntax::set_parameters(ParametersListSyntax* parameters)
{
	m_parameters = parameters;
}

void LambdaExpressionSyntax::set_body(BlockSyntax* body)
{
	m_body = body;
}

TextLocation LambdaExpressionSyntax::get_location() const
{
	if (!(m_body == nullptr))
		return TextLocation(m_parameters->get_open_token(), m_body->get_location());

	return m_parameters->get_location();
}

void LambdaExpressionSyntax::accept(SyntaxVisitor& visitor) const
{
	visitor.visit_lambda_expression(this);
}