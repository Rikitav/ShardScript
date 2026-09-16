#include <shard/parsing/nodes/blocks/ArrowClauseSyntax.hpp>
#include <shard/parsing/SyntaxVisitor.hpp>

#include <gmt/Arena.hpp>

using namespace shard;

ArrowClauseSyntax::ArrowClauseSyntax(gmt::Ref<SyntaxNode> parent)
	: BodySyntax(SyntaxKind::ArrowClause, parent) { }

gmt::Ref<const ExpressionSyntax> ArrowClauseSyntax::get_expression() const
{
	return m_expression;
}

SyntaxToken ArrowClauseSyntax::get_arrow_token() const
{
	return m_arrowToken;
}

void ArrowClauseSyntax::set_expression(gmt::Ref<ExpressionSyntax> expression)
{
	m_expression = expression;
}

void ArrowClauseSyntax::set_arrow_token(const SyntaxToken& token)
{
	m_arrowToken = token;
}

TextLocation ArrowClauseSyntax::get_location() const
{
	if (!m_expression.is_null())
		return TextLocation(m_arrowToken, m_expression.as_ptr()->get_location());

	return m_arrowToken.get_location();
}

void ArrowClauseSyntax::accept(SyntaxVisitor& visitor) const
{
	visitor.visit_arrow_clause(this);
}
