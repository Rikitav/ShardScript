#include <shard/parsing/nodes/expressions/CollectionExpressionSyntax.hpp>
#include <shard/parsing/SyntaxVisitor.hpp>

#include <gmt/Arena.hpp>

using namespace shard;

CollectionExpressionSyntax::CollectionExpressionSyntax(gmt::Ref<SyntaxNode> parent)
	: ExpressionSyntax(SyntaxKind::CollectionExpression, parent) { }

gmt::Span<const gmt::Ref<ExpressionSyntax>> CollectionExpressionSyntax::get_values() const
{
	return m_values;
}

SyntaxToken CollectionExpressionSyntax::get_open_bracket() const
{
	return m_openBracketToken;
}

SyntaxToken CollectionExpressionSyntax::get_close_bracket() const
{
	return m_closeBracketToken;
}

void CollectionExpressionSyntax::set_values(gmt::Span<gmt::Ref<ExpressionSyntax>> values)
{
	m_values = values;
}

void CollectionExpressionSyntax::set_open_bracket(const SyntaxToken& token)
{
	m_openBracketToken = token;
}

void CollectionExpressionSyntax::set_close_bracket(const SyntaxToken& token)
{
	m_closeBracketToken = token;
}

TextLocation CollectionExpressionSyntax::get_location() const
{
	return TextLocation(m_openBracketToken, m_closeBracketToken);
}

void CollectionExpressionSyntax::accept(SyntaxVisitor& visitor) const
{
	visitor.visit_collection_expression(this);
}
