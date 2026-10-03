#include <shard/parsing/nodes/members/PropertyDeclarationSyntax.hpp>
#include <shard/parsing/SyntaxVisitor.hpp>

#include <gmt/Arena.hpp>

using namespace shard;

PropertyDeclarationSyntax::PropertyDeclarationSyntax(gmt::Ref<SyntaxNode> parent)
	: MemberDeclarationSyntax(SyntaxKind::PropertyDeclaration, parent) { }

SyntaxToken PropertyDeclarationSyntax::get_colon() const
{
	return m_colonToken;
}

gmt::Ref<const TypeSyntax> PropertyDeclarationSyntax::get_type() const
{
	return m_type;
}

SyntaxToken PropertyDeclarationSyntax::get_open_bracket() const
{
	return m_openBracketToken;
}

gmt::Ref<const AccessorDeclarationSyntax> PropertyDeclarationSyntax::get_getter() const
{
	return m_getter;
}

gmt::Ref<const AccessorDeclarationSyntax> PropertyDeclarationSyntax::get_setter() const
{
	return m_setter;
}

SyntaxToken PropertyDeclarationSyntax::get_close_bracket() const
{
	return m_closeBracketToken;
}

SyntaxToken PropertyDeclarationSyntax::get_assign_token() const
{
	return m_assignToken;
}

gmt::Ref<const ExpressionSyntax> PropertyDeclarationSyntax::get_expression() const
{
	return m_expression;
}

SyntaxToken PropertyDeclarationSyntax::get_semicolon() const
{
	return m_semicolonToken;
}

void PropertyDeclarationSyntax::set_colon(const SyntaxToken& token)
{
	m_colonToken = token;
}

void PropertyDeclarationSyntax::set_type(gmt::Ref<TypeSyntax> type)
{
	m_type = type;
}

void PropertyDeclarationSyntax::set_open_bracket(const SyntaxToken& token)
{
	m_openBracketToken = token;
}

void PropertyDeclarationSyntax::set_getter(gmt::Ref<AccessorDeclarationSyntax> getter)
{
	m_getter = getter;
}

void PropertyDeclarationSyntax::set_setter(gmt::Ref<AccessorDeclarationSyntax> setter)
{
	m_setter = setter;
}

void PropertyDeclarationSyntax::set_close_bracket(const SyntaxToken& token)
{
	m_closeBracketToken = token;
}

void PropertyDeclarationSyntax::set_assign_token(const SyntaxToken& token)
{
	m_assignToken = token;
}

void PropertyDeclarationSyntax::set_expression(gmt::Ref<ExpressionSyntax> expression)
{
	m_expression = expression;
}

void PropertyDeclarationSyntax::set_semicolon(const SyntaxToken& token)
{
	m_semicolonToken = token;
}

TextLocation PropertyDeclarationSyntax::get_location() const
{
	SyntaxToken identifierToken = get_identifier();

	if (!m_semicolonToken.is_missing())
		return TextLocation(identifierToken, m_semicolonToken);

	if (!m_closeBracketToken.is_missing())
		return TextLocation(identifierToken, m_closeBracketToken);

	if (!m_type.is_null())
		return TextLocation(identifierToken, m_type.as_ptr()->get_location());

	return identifierToken.get_location();
}

void PropertyDeclarationSyntax::accept(SyntaxVisitor& visitor) const
{
	visitor.visit_property_declaration(this);
}
