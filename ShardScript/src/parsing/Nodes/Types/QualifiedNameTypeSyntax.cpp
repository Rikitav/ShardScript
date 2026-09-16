#include <shard/parsing/nodes/types/QualifiedNameTypeSyntax.hpp>
#include <shard/parsing/SyntaxVisitor.hpp>

#include <gmt/Arena.hpp>

#include <format>

using namespace shard;

QualifiedNameTypeSyntax::QualifiedNameTypeSyntax(gmt::Ref<SyntaxNode> parent)
	: TypeSyntax(SyntaxKind::QualifiedNameType, parent) { }

gmt::Ref<const TypeSyntax> QualifiedNameTypeSyntax::get_left() const
{
	return m_left;
}

SyntaxToken QualifiedNameTypeSyntax::get_qualifier_token() const
{
	return m_qualifierToken;
}

SyntaxToken QualifiedNameTypeSyntax::get_identifier() const
{
	return m_identifierToken;
}

void QualifiedNameTypeSyntax::set_left(gmt::Ref<TypeSyntax> left)
{
	m_left = left;
}

void QualifiedNameTypeSyntax::set_qualifier_token(const SyntaxToken& token)
{
	m_qualifierToken = token;
}

void QualifiedNameTypeSyntax::set_identifier(const SyntaxToken& token)
{
	m_identifierToken = token;
}

std::wstring QualifiedNameTypeSyntax::get_qualifier() const
{
	if (m_left.is_null())
		return std::wstring(m_identifierToken.get_lexeme());

	return std::format(L"{}::{}", m_left.get()->get_qualifier(), m_identifierToken.get_lexeme());
}

TextLocation QualifiedNameTypeSyntax::get_location() const
{
	if (!m_left.is_null())
		return TextLocation(m_left.get()->get_location(), m_identifierToken);

	return m_identifierToken.get_location();
}

void QualifiedNameTypeSyntax::accept(SyntaxVisitor& visitor) const
{
	visitor.visit_qualified_name_type(this);
}
