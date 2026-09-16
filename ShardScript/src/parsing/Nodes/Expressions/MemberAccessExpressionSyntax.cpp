#include <shard/parsing/nodes/expressions/MemberAccessExpressionSyntax.hpp>
#include <shard/parsing/SyntaxVisitor.hpp>

#include <gmt/Arena.hpp>

using namespace shard;

MemberAccessExpressionSyntax::MemberAccessExpressionSyntax(gmt::Ref<SyntaxNode> parent)
	: ExpressionSyntax(SyntaxKind::MemberAccessExpression, parent) { }

gmt::Ref<const ExpressionSyntax> MemberAccessExpressionSyntax::get_previous() const
{
	return m_previous;
}

SyntaxToken MemberAccessExpressionSyntax::get_delimeter_token() const
{
	return m_delimeterToken;
}

SyntaxToken MemberAccessExpressionSyntax::get_identifier() const
{
	return m_identifierToken;
}

void MemberAccessExpressionSyntax::set_previous(gmt::Ref<ExpressionSyntax> previous)
{
	m_previous = previous;
}

void MemberAccessExpressionSyntax::set_delimeter_token(const SyntaxToken& token)
{
	m_delimeterToken = token;
}

void MemberAccessExpressionSyntax::set_identifier(const SyntaxToken& token)
{
	m_identifierToken = token;
}

TextLocation MemberAccessExpressionSyntax::get_location() const
{
	if (!m_previous.is_null())
		return TextLocation(m_previous.as_ptr()->get_location(), m_identifierToken);

	return m_identifierToken.get_location();
}

void MemberAccessExpressionSyntax::accept(SyntaxVisitor& visitor) const
{
	visitor.visit_member_access_expression(this);
}
