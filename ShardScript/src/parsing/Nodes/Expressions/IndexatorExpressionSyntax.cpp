#include <shard/parsing/nodes/expressions/IndexatorExpressionSyntax.hpp>
#include <shard/parsing/SyntaxVisitor.hpp>

#include <gmt/Arena.hpp>

using namespace shard;

IndexatorExpressionSyntax::IndexatorExpressionSyntax(SyntaxNode* parent)
	: ExpressionSyntax(SyntaxKind::IndexatorExpression, parent) { }

const ExpressionSyntax* IndexatorExpressionSyntax::get_previous() const
{
	return m_previous;
}

SyntaxToken IndexatorExpressionSyntax::get_delimeter_token() const
{
	return m_delimeterToken;
}

const ArgumentsListSyntax* IndexatorExpressionSyntax::get_arguments() const
{
	return m_arguments;
}

void IndexatorExpressionSyntax::set_previous(ExpressionSyntax* previous)
{
	m_previous = previous;
}

void IndexatorExpressionSyntax::set_delimeter_token(const SyntaxToken& token)
{
	m_delimeterToken = token;
}

void IndexatorExpressionSyntax::set_arguments(ArgumentsListSyntax* arguments)
{
	m_arguments = arguments;
}

TextLocation IndexatorExpressionSyntax::get_location() const
{
	if (!(m_previous == nullptr))
	{
		if (!(m_arguments == nullptr))
			return TextLocation(m_previous->get_location(), m_arguments->get_close_token());

		return TextLocation(m_previous->get_location(), m_delimeterToken);
	}

	if (!(m_arguments == nullptr))
		return m_arguments->get_location();

	return m_delimeterToken.get_location();
}

void IndexatorExpressionSyntax::accept(SyntaxVisitor& visitor) const
{
	visitor.visit_indexator_expression(this);
}