#include <shard/parsing/nodes/expressions/LinkedExpressionSyntax.hpp>
#include <shard/parsing/SyntaxVisitor.hpp>

#include <gmt/Arena.hpp>

using namespace shard;

LinkedExpressionNode::LinkedExpressionNode(const SyntaxKind kind, gmt::Ref<SyntaxNode> parent)
	: ExpressionSyntax(kind, parent) { }

gmt::Ref<const ExpressionSyntax> LinkedExpressionNode::get_previous() const
{
	return m_previous;
}

SyntaxToken LinkedExpressionNode::get_delimeter_token() const
{
	return m_delimeterToken;
}

void LinkedExpressionNode::set_previous(gmt::Ref<ExpressionSyntax> previous)
{
	m_previous = previous;
}

void LinkedExpressionNode::set_delimeter_token(const SyntaxToken& token)
{
	m_delimeterToken = token;
}

TextLocation LinkedExpressionNode::get_location() const
{
	if (!m_previous.is_null())
		return m_previous.get()->get_location();

	return m_delimeterToken.get_location();
}

MemberAccessExpressionSyntax::MemberAccessExpressionSyntax(gmt::Ref<SyntaxNode> parent)
	: LinkedExpressionNode(SyntaxKind::MemberAccessExpression, parent) { }

SyntaxToken MemberAccessExpressionSyntax::get_identifier() const
{
	return m_identifierToken;
}

void MemberAccessExpressionSyntax::set_identifier(const SyntaxToken& token)
{
	m_identifierToken = token;
}

void MemberAccessExpressionSyntax::accept(SyntaxVisitor& visitor) const
{
	visitor.visit_member_access_expression(this);
}

InvokationExpressionSyntax::InvokationExpressionSyntax(gmt::Ref<SyntaxNode> parent)
	: LinkedExpressionNode(SyntaxKind::InvokationExpression, parent) { }

SyntaxToken InvokationExpressionSyntax::get_identifier() const
{
	return m_identifierToken;
}

gmt::Ref<const ArgumentsListSyntax> InvokationExpressionSyntax::get_arguments() const
{
	return m_arguments;
}

void InvokationExpressionSyntax::set_identifier(const SyntaxToken& token)
{
	m_identifierToken = token;
}

void InvokationExpressionSyntax::set_arguments(gmt::Ref<ArgumentsListSyntax> arguments)
{
	m_arguments = arguments;
}

void InvokationExpressionSyntax::accept(SyntaxVisitor& visitor) const
{
	visitor.visit_invokation_expression(this);
}
