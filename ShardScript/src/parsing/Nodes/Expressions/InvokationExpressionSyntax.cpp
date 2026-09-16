#include <shard/parsing/nodes/expressions/InvokationExpressionSyntax.hpp>
#include <shard/parsing/SyntaxVisitor.hpp>

#include <gmt/Arena.hpp>

using namespace shard;

InvokationExpressionSyntax::InvokationExpressionSyntax(gmt::Ref<SyntaxNode> parent)
	: ExpressionSyntax(SyntaxKind::InvokationExpression, parent) { }

gmt::Ref<const ExpressionSyntax> InvokationExpressionSyntax::get_previous() const
{
	return m_previous;
}

SyntaxToken InvokationExpressionSyntax::get_delimeter_token() const
{
	return m_delimeterToken;
}

SyntaxToken InvokationExpressionSyntax::get_identifier() const
{
	return m_identifierToken;
}

gmt::Ref<const ArgumentsListSyntax> InvokationExpressionSyntax::get_arguments() const
{
	return m_arguments;
}

void InvokationExpressionSyntax::set_previous(gmt::Ref<ExpressionSyntax> previous)
{
	m_previous = previous;
}

void InvokationExpressionSyntax::set_delimeter_token(const SyntaxToken& token)
{
	m_delimeterToken = token;
}

void InvokationExpressionSyntax::set_identifier(const SyntaxToken& token)
{
	m_identifierToken = token;
}

void InvokationExpressionSyntax::set_arguments(gmt::Ref<ArgumentsListSyntax> arguments)
{
	m_arguments = arguments;
}

TextLocation InvokationExpressionSyntax::get_location() const
{
	if (!m_previous.is_null())
	{
		if (!m_arguments.is_null())
			return TextLocation(m_previous.as_ptr()->get_location(), m_arguments.as_ptr()->get_close_token());

		return TextLocation(m_previous.as_ptr()->get_location(), m_identifierToken);
	}

	if (!m_arguments.is_null())
		return TextLocation(m_identifierToken, m_arguments.as_ptr()->get_location());

	return m_identifierToken.get_location();
}

void InvokationExpressionSyntax::accept(SyntaxVisitor& visitor) const
{
	visitor.visit_invokation_expression(this);
}
