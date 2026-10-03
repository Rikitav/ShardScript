#include <shard/parsing/nodes/members/AccessorDeclarationSyntax.hpp>
#include <shard/parsing/SyntaxVisitor.hpp>

#include <gmt/Arena.hpp>

using namespace shard;

AccessorDeclarationSyntax::AccessorDeclarationSyntax(gmt::Ref<SyntaxNode> parent)
	: MemberDeclarationSyntax(SyntaxKind::AccessorDeclaration, parent) { }

SyntaxToken AccessorDeclarationSyntax::get_keyword() const
{
	return m_keywordToken;
}

SyntaxToken AccessorDeclarationSyntax::get_semicolon() const
{
	return m_semicolonToken;
}

void AccessorDeclarationSyntax::set_keyword(const SyntaxToken& token)
{
	m_keywordToken = token;
}

void AccessorDeclarationSyntax::set_semicolon(const SyntaxToken& token)
{
	m_semicolonToken = token;
}

TextLocation AccessorDeclarationSyntax::get_location() const
{
	if (!m_semicolonToken.is_missing())
		return TextLocation(m_keywordToken, m_semicolonToken);

	return m_keywordToken.get_location();
}

void AccessorDeclarationSyntax::accept(SyntaxVisitor& visitor) const
{
	visitor.visit_accessor_declaration(this);
}
