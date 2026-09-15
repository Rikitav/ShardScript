#include <shard/parsing/nodes/types/IdentifierNameTypeSyntax.hpp>
#include <shard/parsing/SyntaxVisitor.hpp>

#include <gmt/Arena.hpp>

using namespace shard;

IdentifierNameTypeSyntax::IdentifierNameTypeSyntax(gmt::Ref<SyntaxNode> parent)
	: TypeSyntax(SyntaxKind::IdentifierNameType, parent) { }

SyntaxToken IdentifierNameTypeSyntax::get_identifier() const
{
	return m_identifierToken;
}

void IdentifierNameTypeSyntax::set_identifier(const SyntaxToken& token)
{
	m_identifierToken = token;
}

std::wstring IdentifierNameTypeSyntax::get_qualifier() const
{
	return m_identifierToken.get_lexeme();
}

TextLocation IdentifierNameTypeSyntax::get_location() const
{
	return m_identifierToken.get_location();
}

void IdentifierNameTypeSyntax::accept(SyntaxVisitor& visitor) const
{
	visitor.visit_identifier_name_type(this);
}
