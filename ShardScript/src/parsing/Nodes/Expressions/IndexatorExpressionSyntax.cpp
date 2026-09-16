#include <shard/parsing/nodes/expressions/IndexatorExpressionSyntax.hpp>
#include <shard/parsing/SyntaxVisitor.hpp>

#include <gmt/Arena.hpp>

using namespace shard;

IndexatorExpressionSyntax::IndexatorExpressionSyntax(gmt::Ref<SyntaxNode> parent)
	: ExpressionSyntax(SyntaxKind::IndexatorExpression, parent) { }

gmt::Ref<const ExpressionSyntax> IndexatorExpressionSyntax::get_previous() const
{
	return m_previous;
}

SyntaxToken IndexatorExpressionSyntax::get_delimeter_token() const
{
	return m_delimeterToken;
}

gmt::Ref<const ArgumentsListSyntax> IndexatorExpressionSyntax::get_arguments() const
{
	return m_arguments;
}

void IndexatorExpressionSyntax::set_previous(gmt::Ref<ExpressionSyntax> previous)
{
	m_previous = previous;
}

void IndexatorExpressionSyntax::set_delimeter_token(const SyntaxToken& token)
{
	m_delimeterToken = token;
}

void IndexatorExpressionSyntax::set_arguments(gmt::Ref<ArgumentsListSyntax> arguments)
{
	m_arguments = arguments;
}

TextLocation IndexatorExpressionSyntax::get_location() const
{
	if (!m_previous.is_null())
	{
		if (!m_arguments.is_null())
			return TextLocation(m_previous.as_ptr()->get_location(), m_arguments.as_ptr()->get_close_token());

		return TextLocation(m_previous.as_ptr()->get_location(), m_delimeterToken);
	}

	if (!m_arguments.is_null())
		return m_arguments.as_ptr()->get_location();

	return m_delimeterToken.get_location();
}

void IndexatorExpressionSyntax::accept(SyntaxVisitor& visitor) const
{
	visitor.visit_indexator_expression(this);
}
