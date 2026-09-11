#include <shard/parsing/nodes/Directives/NamespaceDirectiveSyntax.hpp>
#include <shard/parsing/SyntaxVisitor.hpp>

#include <string>
#include <sstream>
#include <iostream>

using namespace shard;

NamespaceDirectiveSyntax::NamespaceDirectiveSyntax(SyntaxNode* parent)
	: SyntaxNode(SyntaxKind::NamespaceDeclaration, parent) { }

std::span<const SyntaxToken> NamespaceDirectiveSyntax::get_qualifier() const
{
	return std::span<const SyntaxToken>(m_qualifier.data(), m_qualifier.data() + m_qualifier.size());
}

string_t NamespaceDirectiveSyntax::get_qualifier_string() const
{
	NamespaceDirectiveSyntax* self = const_cast<NamespaceDirectiveSyntax*>(this);
	if (!m_qualifierStringCache.empty() && !m_changed)
		return m_qualifierStringCache.c_str();

	if (m_qualifier.empty())
		return L"";

	std::wstringstream qualifierString;
	for (std::size_t i = 0; i < m_qualifier.size() - 1; ++i)
		qualifierString << self->m_qualifier.at(i).get_lexeme() << L".";

	qualifierString << m_qualifier.back().get_lexeme();
	self->m_qualifierStringCache = qualifierString.str();
	self->m_changed = false;

	return m_qualifierStringCache.c_str();
}

SyntaxToken NamespaceDirectiveSyntax::get_semicolon() const
{
	return m_semicolon;
}

SyntaxToken NamespaceDirectiveSyntax::get_namespace_keyword() const
{
	return m_namespaceKeyword;
}

void NamespaceDirectiveSyntax::add_qulifier(const SyntaxToken& token)
{
	m_qualifier.push_back(token);
	m_changed = true;
}

void NamespaceDirectiveSyntax::set_namespace_keyword(const SyntaxToken& token)
{
	m_namespaceKeyword = token;
}

void NamespaceDirectiveSyntax::set_semicolon(const SyntaxToken& token)
{
	m_semicolon = token;
}

TextLocation NamespaceDirectiveSyntax::get_location() const
{
	return TextLocation(m_namespaceKeyword, m_semicolon);
}

void NamespaceDirectiveSyntax::accept(SyntaxVisitor& visitor) const
{
	visitor.visit_namespace_directive(this);
}
