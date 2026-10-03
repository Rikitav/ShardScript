#include <shard/parsing/nodes/members/FieldDeclarationSyntax.hpp>
#include <shard/parsing/SyntaxVisitor.hpp>

#include <gmt/Arena.hpp>

using namespace shard;

FieldDeclarationSyntax::FieldDeclarationSyntax(gmt::Ref<SyntaxNode> parent)
	: MemberDeclarationSyntax(SyntaxKind::FieldDeclaration, parent) { }

SyntaxToken FieldDeclarationSyntax::get_colon() const
{
	return m_colonToken;
}

gmt::Ref<const TypeSyntax> FieldDeclarationSyntax::get_type() const
{
	return m_type;
}

SyntaxToken FieldDeclarationSyntax::get_assign_token() const
{
	return m_assignToken;
}

gmt::Ref<const ExpressionSyntax> FieldDeclarationSyntax::get_expression() const
{
	return m_expression;
}

SyntaxToken FieldDeclarationSyntax::get_semicolon() const
{
	return m_semicolonToken;
}

void FieldDeclarationSyntax::set_colon(const SyntaxToken& token)
{
	m_colonToken = token;
}

void FieldDeclarationSyntax::set_type(gmt::Ref<TypeSyntax> type)
{
	m_type = type;
}

void FieldDeclarationSyntax::set_assign_token(const SyntaxToken& token)
{
	m_assignToken = token;
}

void FieldDeclarationSyntax::set_expression(gmt::Ref<ExpressionSyntax> expression)
{
	m_expression = expression;
}

void FieldDeclarationSyntax::set_semicolon(const SyntaxToken& token)
{
	m_semicolonToken = token;
}

TextLocation FieldDeclarationSyntax::get_location() const
{
	SyntaxToken identifierToken = get_identifier();

	if (!m_semicolonToken.is_missing())
		return TextLocation(identifierToken, m_semicolonToken);

	if (!m_type.is_null())
		return TextLocation(identifierToken, m_type.as_ptr()->get_location());

	return identifierToken.get_location();
}

void FieldDeclarationSyntax::accept(SyntaxVisitor& visitor) const
{
	visitor.visit_field_declaration(this);
}
