#include <shard/parsing/nodes/MemberDeclarationSyntax.hpp>

#include <gmt/Arena.hpp>

using namespace shard;

MemberDeclarationSyntax::MemberDeclarationSyntax(const SyntaxKind kind, SyntaxNode* parent)
	: SyntaxNode(kind, parent) { }

void MemberDeclarationSyntax::set_attributes(AttributesListSyntax* attributes)
{
	m_attributes = attributes;
}

void MemberDeclarationSyntax::set_modifiers(std::span<SyntaxToken> modifiers)
{
	m_modifiers = modifiers;
}

void MemberDeclarationSyntax::set_identifier(const SyntaxToken& identifier)
{
	m_identifierToken = identifier;
}

void MemberDeclarationSyntax::set_declare_token(const SyntaxToken& declare)
{
	m_declareToken = declare;
}

AttributesListSyntax* MemberDeclarationSyntax::get_attributes() const
{
	return m_attributes;
}

std::span<const SyntaxToken> MemberDeclarationSyntax::get_modifiers() const
{
	return m_modifiers;
}

SyntaxToken MemberDeclarationSyntax::get_identifier() const
{
	return m_identifierToken;
}

SyntaxToken MemberDeclarationSyntax::get_declare_token() const
{
	return m_declareToken;
}