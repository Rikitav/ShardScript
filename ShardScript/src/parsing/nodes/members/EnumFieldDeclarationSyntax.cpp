#include <shard/parsing/nodes/members/EnumFieldDeclarationSyntax.hpp>
#include <shard/parsing/SyntaxVisitor.hpp>

#include <gmt/Arena.hpp>

using namespace shard;

EnumFieldDeclarationSyntax::EnumFieldDeclarationSyntax(gmt::Ref<SyntaxNode> parent)
	: SyntaxNode(SyntaxKind::EnumFieldDeclaration, parent) { }

SyntaxToken EnumFieldDeclarationSyntax::get_identifier() const
{
	return m_identifierToken;
}

SyntaxToken EnumFieldDeclarationSyntax::get_assign_token() const
{
	return m_assignToken;
}

gmt::Ref<const ExpressionSyntax> EnumFieldDeclarationSyntax::get_expression() const
{
	return m_expression;
}

void EnumFieldDeclarationSyntax::set_identifier(const SyntaxToken& token)
{
	m_identifierToken = token;
}

void EnumFieldDeclarationSyntax::set_assign_token(const SyntaxToken& token)
{
	m_assignToken = token;
}

void EnumFieldDeclarationSyntax::set_expression(gmt::Ref<ExpressionSyntax> expression)
{
	m_expression = expression;
}

TextLocation EnumFieldDeclarationSyntax::get_location() const
{
	SyntaxToken identifierToken = get_identifier();

	if (!m_expression.is_null())
		return TextLocation(identifierToken, m_expression.as_ptr()->get_location());

	if (!m_assignToken.is_missing())
		return TextLocation(identifierToken, m_assignToken);

	return identifierToken.get_location();
}

void EnumFieldDeclarationSyntax::accept(SyntaxVisitor& visitor) const
{
	visitor.visit_enum_field_declaration(this);
}
