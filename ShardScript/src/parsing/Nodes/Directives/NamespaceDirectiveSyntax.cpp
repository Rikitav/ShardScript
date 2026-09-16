#include <shard/parsing/nodes/directives/NamespaceDirectiveSyntax.hpp>
#include <shard/parsing/SyntaxVisitor.hpp>

#include <gmt/Arena.hpp>

#include <sstream>

using namespace shard;

NamespaceDirectiveSyntax::NamespaceDirectiveSyntax(gmt::Ref<SyntaxNode> parent)
	: SyntaxNode(SyntaxKind::NamespaceDirective, parent) { }

gmt::Span<const SyntaxToken> NamespaceDirectiveSyntax::get_qualifier() const
{
	return m_qualifier;
}

std::wstring NamespaceDirectiveSyntax::get_qualifier_string() const
{
	std::span<const SyntaxToken> qualifier = m_qualifier.as_span();
	if (qualifier.empty())
		return L"";

	std::wstringstream qualifierString;
	for (std::size_t i = 0; i < qualifier.size() - 1; ++i)
		qualifierString << qualifier[i].get_lexeme() << L".";

	qualifierString << qualifier.back().get_lexeme();
	return qualifierString.str();
}

SyntaxToken NamespaceDirectiveSyntax::get_semicolon() const
{
	return m_semicolon;
}

SyntaxToken NamespaceDirectiveSyntax::get_namespace_keyword() const
{
	return m_namespaceKeyword;
}

void NamespaceDirectiveSyntax::set_qualifier(gmt::Span<SyntaxToken> qualifier)
{
	m_qualifier = qualifier;
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
