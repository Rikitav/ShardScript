#include <shard/parsing/nodes/members/PropertyDeclarationSyntax.hpp>
#include <shard/parsing/SyntaxVisitor.hpp>

#include <gmt/Arena.hpp>

using namespace shard;

PropertyDeclarationSyntax::PropertyDeclarationSyntax(SyntaxNode* parent)
	: MemberDeclarationSyntax(SyntaxKind::PropertyDeclaration, parent) { }

SyntaxToken PropertyDeclarationSyntax::get_arrow() const
{
	return m_arrowToken;
}

const TypeSyntax* PropertyDeclarationSyntax::get_type() const
{
	return m_type;
}

SyntaxToken PropertyDeclarationSyntax::get_open_bracket() const
{
	return m_openBracketToken;
}

const AccessorDeclarationSyntax* PropertyDeclarationSyntax::get_getter() const
{
	return m_getter;
}

const AccessorDeclarationSyntax* PropertyDeclarationSyntax::get_setter() const
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

const ExpressionSyntax* PropertyDeclarationSyntax::get_expression() const
{
	return m_expression;
}

SyntaxToken PropertyDeclarationSyntax::get_semicolon() const
{
	return m_semicolonToken;
}

void PropertyDeclarationSyntax::set_arrow(const SyntaxToken& token)
{
	m_arrowToken = token;
}

void PropertyDeclarationSyntax::set_type(TypeSyntax* type)
{
	m_type = type;
}

void PropertyDeclarationSyntax::set_open_bracket(const SyntaxToken& token)
{
	m_openBracketToken = token;
}

void PropertyDeclarationSyntax::set_getter(AccessorDeclarationSyntax* getter)
{
	m_getter = getter;
}

void PropertyDeclarationSyntax::set_setter(AccessorDeclarationSyntax* setter)
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

void PropertyDeclarationSyntax::set_expression(ExpressionSyntax* expression)
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

	if (!(m_type == nullptr))
		return TextLocation(identifierToken, m_type->get_location());

	return identifierToken.get_location();
}

void PropertyDeclarationSyntax::accept(SyntaxVisitor& visitor) const
{
	visitor.visit_property_declaration(this);
}