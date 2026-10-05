#include <shard/parsing/nodes/members/EnumDeclarationSyntax.hpp>
#include <shard/parsing/SyntaxVisitor.hpp>

#include <gmt/Arena.hpp>

using namespace shard;

EnumDeclarationSyntax::EnumDeclarationSyntax(gmt::Ref<SyntaxNode> parent)
	: MemberDeclarationSyntax(SyntaxKind::EnumDeclaration, parent)
	, m_isFlags(false) { }

SyntaxToken EnumDeclarationSyntax::get_colon() const
{
	return m_colonToken;
}

SyntaxToken EnumDeclarationSyntax::get_underlying_type() const
{
	return m_underlyingTypeToken;
}

bool EnumDeclarationSyntax::get_is_flags() const
{
	return m_isFlags;
}

SyntaxToken EnumDeclarationSyntax::get_open_bracket() const
{
	return m_openBracketToken;
}

SyntaxToken EnumDeclarationSyntax::get_close_bracket() const
{
	return m_closeBracketToken;
}

SyntaxToken EnumDeclarationSyntax::get_semicolon() const
{
	return m_semicolonToken;
}

gmt::Span<const gmt::Ref<EnumFieldDeclarationSyntax>> EnumDeclarationSyntax::get_fields() const
{
	return m_fields;
}

void EnumDeclarationSyntax::set_colon(const SyntaxToken& token)
{
	m_colonToken = token;
}

void EnumDeclarationSyntax::set_underlying_type(const SyntaxToken& token)
{
	m_underlyingTypeToken = token;
}

void EnumDeclarationSyntax::set_is_flags(bool isFlags)
{
	m_isFlags = isFlags;
}

void EnumDeclarationSyntax::set_open_bracket(const SyntaxToken& token)
{
	m_openBracketToken = token;
}

void EnumDeclarationSyntax::set_close_bracket(const SyntaxToken& token)
{
	m_closeBracketToken = token;
}

void EnumDeclarationSyntax::set_semicolon(const SyntaxToken& token)
{
	m_semicolonToken = token;
}

void EnumDeclarationSyntax::set_fields(gmt::Span<gmt::Ref<EnumFieldDeclarationSyntax>> fields)
{
	m_fields = fields;
}

TextLocation EnumDeclarationSyntax::get_location() const
{
	TextLocation location = get_declare_token().get_location();

	if (!get_close_bracket().is_missing())
		return TextLocation(location, get_close_bracket().get_location());

	if (!get_semicolon().is_missing())
		return TextLocation(location, get_semicolon().get_location());

	if (!get_underlying_type().is_missing())
		return TextLocation(location, get_underlying_type().get_location());

	if (!get_identifier().is_missing())
		return TextLocation(location, get_identifier().get_location());

	return location;
}

void EnumDeclarationSyntax::accept(SyntaxVisitor& visitor) const
{
	visitor.visit_enum_declaration(this);
}
