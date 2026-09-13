#include <shard/parsing/nodes/MemberDeclarationSyntax.hpp>
#include <gmt/Arena.hpp>

using namespace shard;

MemberDeclarationSyntax::MemberDeclarationSyntax(const SyntaxKind kind, gmt::Ref<SyntaxNode> parent)
	: SyntaxNode(kind, parent) { }

void MemberDeclarationSyntax::set_attributes(gmt::Span<gmt::Ref<AttributeSyntax>> attributes)
{
	m_attributes = attributes;
}

void MemberDeclarationSyntax::set_modifiers(gmt::Span<SyntaxToken> modifiers)
{
	m_modifiers = modifiers;
}

void MemberDeclarationSyntax::set_identifier(const SyntaxToken& identifier)
{
	m_identifierToken = identifier;
}

gmt::Span<const gmt::Ref<AttributeSyntax>> MemberDeclarationSyntax::get_attributes() const
{
	return m_attributes;
}

gmt::Span<const SyntaxToken> MemberDeclarationSyntax::get_modifiers() const
{
	return m_modifiers;
}

SyntaxToken MemberDeclarationSyntax::get_identifier() const
{
	return m_identifierToken;
}

TextLocation MemberDeclarationSyntax::get_location() const
{
	return m_identifierToken.get_location();
}

void MemberDeclarationSyntax::accept(SyntaxVisitor& visitor) const
{
}
